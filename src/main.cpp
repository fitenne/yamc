#include <fcntl.h>
#include <glog/logging.h>
#include <grp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "config.h"
#include "jail.h"
#include "utils.h"

static bool createWorkingDir(const yamc::fs::path &root) {
    if (!yamc::fs::exists(root)) {
        DLOG(INFO) << "creating new root directory: "
                   << yamc::fs::absolute(root);
        yamc::fs::create_directories(root);
        return true;
    }
    return false;
}

static void fakeRoot(const yamc::Config &conf) {
    static const auto dummyFun = [](void *) -> int {
        pause();
        return 0;
    };
    static const int dummyFun_stack_size = 8 * 1024;

    auto ruid = getuid();
    auto rgid = getgid();
    std::vector<yamc::IDMap> uidmap{{0, ruid, 1}};
    std::vector<yamc::IDMap> gidmap{{0, rgid, 1}};

    // additional id map
    const auto &uu = conf.use_uid;
    const auto &ug = conf.use_gid;
    if (uu.outside_id != ruid) {
        uidmap.emplace_back(uu.inside_id, uu.outside_id, uu.count);
    }
    if (ug.outside_id != rgid) {
        gidmap.emplace_back(ug.inside_id, ug.outside_id, ug.count);
    }

    // execute new{u,g}idmap
    uint8_t *stack =
        (uint8_t *)mmap(nullptr, dummyFun_stack_size, PROT_WRITE | PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    try {
        auto dummy_worker = clone(dummyFun, stack + dummyFun_stack_size,
                                  CLONE_NEWUSER, nullptr);
        if (dummy_worker == -1) {
            LOG(ERROR) << "failed to call clone";
            throw std::runtime_error(strerror(errno));
        }

        std::vector<std::string> uidmapcmd;
        uidmapcmd.emplace_back("/usr/bin/newuidmap");
        uidmapcmd.emplace_back(std::to_string(dummy_worker));
        for (const auto &m : uidmap) {
            uidmapcmd.emplace_back(std::to_string(m.inside_id));
            uidmapcmd.emplace_back(std::to_string(m.outside_id));
            uidmapcmd.emplace_back(std::to_string(m.count));
        }
        auto uid_wkr = yamc::systemExec(uidmapcmd);

        std::vector<std::string> gidmapcmd;
        gidmapcmd.emplace_back("/usr/bin/newgidmap");
        gidmapcmd.emplace_back(std::to_string(dummy_worker));
        for (const auto &m : gidmap) {
            gidmapcmd.emplace_back(std::to_string(m.inside_id));
            gidmapcmd.emplace_back(std::to_string(m.outside_id));
            gidmapcmd.emplace_back(std::to_string(m.count));
        }
        auto gid_wkr = yamc::systemExec(gidmapcmd);

        int uid_wkrsta, gid_wkrsta;
        if (waitpid(uid_wkr, &uid_wkrsta, 0) == -1 ||
            waitpid(gid_wkr, &gid_wkrsta, 0) == -1) {
            LOG(ERROR) << "failed to wait {u,g}id worker process";
            throw std::runtime_error(strerror(errno));
        }
        if ((!WIFEXITED(uid_wkrsta) || WEXITSTATUS(uid_wkrsta) != 0) ||
            (!WIFEXITED(gid_wkrsta) || WEXITSTATUS(gid_wkrsta) != 0)) {
            throw std::runtime_error("newidmap unexpectedly exited");
        }

        auto user_ns = yamc::fs::path("/proc") / std::to_string(dummy_worker) /
                       "ns" / "user";
        yamc::moveToNS(user_ns);

        int status;
        if (kill(dummy_worker, SIGKILL) == -1 ||
            waitpid(dummy_worker, &status, __WALL) != dummy_worker) {
            throw std::runtime_error("failed to kill a dummy process");
        }
    } catch (const std::exception &e) {
        munmap(stack, dummyFun_stack_size);
        throw;
    }
    munmap(stack, dummyFun_stack_size);

    if (setuid(0) == -1) {
        throw std::runtime_error("failed to become fake root");
    }
}

int main(int argc, char *argv[]) {
    FLAGS_logtostderr = true;
    google::InitGoogleLogging(argv[0]);
    auto conf = yamc::parseOptions(argc, argv);

    DLOG(INFO) << "pro: " << conf.cmdline[0] << "  "
               << "real: " << conf.real_time_limit.count() << "  "
               << "cpu: " << conf.cpu_time_limit.count() << "  "
               << "uid: " << conf.use_uid.outside_id << "  "
               << "gid: " << conf.use_gid.outside_id;

    try {
        createWorkingDir(conf.chroot_path);

        fakeRoot(conf);

        yamc::Jail jail{conf};
        auto exit_code = jail.run();
        DLOG(INFO) << "jail exit code: " << exit_code;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
    }

    DLOG(INFO) << "cleaning up " << conf.chroot_path;
    std::error_code ec;
    if (!std::filesystem::remove(conf.chroot_path, ec)) {
        DLOG(ERROR) << "failed to remove " << conf.chroot_path << ": "
                    << ec.message();
    }
    return EXIT_SUCCESS;
}