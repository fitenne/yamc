#include "jail.h"

#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <grp.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "timer.h"
#include "utils.h"

namespace yamc {

Jail::Jail(const Config &config) : conf_(config), jail_pid_(0), jailed_pid_(0) {
    int sock_fd[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sock_fd) == -1) {
        RAW_LOG(ERROR, "failed to create socketpair");
        throw std::runtime_error(strerror(errno));
    }
    sock_inside_ = sock_fd[1];
    sock_outside_ = sock_fd[0];
    timer_fd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    oom_notifier_fd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    cgroup_.regOOMNotifier(oom_notifier_fd_);
}

int Jail::cloneWorkerProc_(void *_jail) {
    // jail process should run in new pid namespace. i.e. pid 1
    auto jail = static_cast<Jail *>(_jail);
    try {
        jail->jailed_pid_ = fork();
        if (jail->jailed_pid_ == -1) {
            throw std::runtime_error(strerror(errno));
        }
        if (jail->jailed_pid_ == 0) {
            // jailed proc
            jail->inJailed_();
            // unreachable code
            exit(EXIT_FAILURE);
        }
        RAW_DLOG(INFO, "see jailed proc as pid: %d", jail->jailed_pid_);
        jail->waitJailed_();
    } catch (const std::exception &e) {
        RAW_LOG(ERROR, "error in jail thread: %s", e.what());
        return EXIT_FAILURE;
    }
    RAW_DLOG(INFO, "cloneWorkerThread exited");
    return EXIT_SUCCESS;
}

void Jail::pivotRoot_() {
    try {
        RAW_DLOG(INFO, "chrooting to %s...", conf_.chroot_path.c_str());
        if (mount("", conf_.chroot_path.c_str(), "tmpfs", 0, "size=16777216") ==
            -1) {
            RAW_LOG(ERROR, "failed to remount chroot %s",
                    conf_.chroot_path.c_str());
            throw std::runtime_error(strerror(errno));
        }

        for (const auto &bind : conf_.robind) {
            // const auto &target =
            //     conf_.chroot_path / bind.dest.lexically_relative("/");
            RAW_DLOG(INFO, "ro mounting %s -> %s", bind.src.c_str(),
                     bind.dest.c_str());
            mountFs(bind, conf_.chroot_path, MS_NOSUID);
            // bindMount(pair.first, target, MS_NOSUID | MS_RDONLY);
            // RAW_DLOG(INFO, "ro mounted %s -> %s", pair.first.c_str(),
            //          target.c_str());
        }
        for (const auto &bind : conf_.rwbind) {
            const auto &target =
                conf_.chroot_path / bind.dest.lexically_relative("/");
            RAW_DLOG(INFO, "ro mounting %s -> %s", bind.src.c_str(),
                     bind.dest.c_str());
            mountFs(bind, conf_.chroot_path, MS_NOSUID);
            // roBind(bind.src, bind.dest, MS_NOSUID);
            // bindMount(pair.first, target, MS_NOSUID);
            // RAW_DLOG(INFO, "rw mounted %s -> %s", pair.first.c_str(),
            //          target.c_str());
        }
        for (const auto &bind : conf_.tmpfs) {
            RAW_DLOG(INFO, "mounting tmpfs %s", bind.dest.c_str());
            mountFs(bind, conf_.chroot_path, MS_NOSUID);
        }

        const auto proc = conf_.chroot_path / "proc";
        fs::create_directory(proc);
        if (mount("", proc.c_str(), "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV,
                  "") == -1) {
            RAW_DLOG(INFO, "failed to mount procfs");
            throw std::runtime_error(strerror(errno));
        }

        auto putold = conf_.chroot_path / "putold";
        fs::create_directory(putold);
        if (pivot_root(conf_.chroot_path.c_str(), putold.c_str()) == -1 ||
            chdir("/") == -1 || umount2("putold", MNT_DETACH) == -1 ||
            !fs::remove("putold")) {
            RAW_LOG(ERROR, "failed to pivot_root");
            throw std::runtime_error(strerror(errno));
        }

        if (mount("", "/", "", MS_REMOUNT | MS_REC | MS_RDONLY | MS_NOSUID,
                  "") == -1) {
            RAW_LOG(ERROR, "failed to remount root");
            throw std::runtime_error(strerror(errno));
        }
    } catch (const std::exception &e) {
        RAW_LOG(ERROR, "failed to pivot root: %s", e.what());
        throw;
    }
}

void Jail::inJailed_() {
    RAW_DLOG(INFO, "jailed process started");

    std::vector<const char *> arg_helper, env_helper;
    try {
        if (unshare(CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWNET | CLONE_NEWIPC |
                    CLONE_NEWCGROUP) == -1) {
            RAW_LOG(ERROR, "failed to unshare some namespace");
            throw std::runtime_error(strerror(errno));
        }

        pivotRoot_();

        if (chdir(conf_.chdir_path.c_str()) == -1) {
            RAW_LOG(ERROR, "failed to chdir to %s", conf_.chdir_path.c_str());
            throw std::runtime_error(strerror(errno));
        }

        if (sethostname(conf_.uts.hostname.c_str(),
                        sizeof(conf_.uts.hostname)) == -1 ||
            setdomainname(conf_.uts.domainname.c_str(),
                          sizeof(conf_.uts.domainname)) == -1) {
            RAW_LOG(ERROR, "failed to set uts name");
            throw std::runtime_error(strerror(errno));
        }

        if (conf_.stdout_fd > 0) {
            RAW_DLOG(INFO, "redirect %d to %d", STDOUT_FILENO, conf_.stdout_fd);
            if (dup3(conf_.stdout_fd, STDOUT_FILENO, 0) == -1) {
                RAW_LOG(ERROR, "failed to redirect stdout to fd %d",
                        conf_.stdout_fd);
                throw std::runtime_error(strerror(errno));
            }
        }

        // credentials with less privilege ?
        if (conf_.use_gid.outside_id != IDMap::NOID) {
            if (setgid(conf_.use_gid.inside_id) == -1) {
                RAW_LOG(ERROR, "failed to set gid inside jail");
                throw std::runtime_error(strerror(errno));
            }
        }
        if (conf_.use_uid.outside_id != IDMap::NOID) {
            if (setuid(conf_.use_uid.inside_id) == -1) {
                RAW_LOG(ERROR, "failed to set uid inside jail");
                throw std::runtime_error(strerror(errno));
            }
        }

        setrlimits_();

        arg_helper = strvec2cstr(conf_.cmdline);
        env_helper = strvec2cstr(conf_.env);
        sendTo_(SOCK::OUTSIDE, MESSAGE::READY);
        recvFrom_(SOCK::OUTSIDE);  // should be MESSAGE::RUN
    } catch (const std::exception &e) {
        RAW_LOG(ERROR, "error in jailed process: %s", strerror(errno));
        sendTo_(SOCK::OUTSIDE, MESSAGE::ERROR);
        exit(EXIT_FAILURE);
    }

    RAW_DLOG(INFO, "execving %s", arg_helper[0]);
    execvpe(arg_helper[0], (char *const *)arg_helper.data(),
            (char *const *)env_helper.data());

    RAW_LOG(ERROR, "failed to call execv: %s", strerror(errno));
}

void Jail::waitJailed_() {
    Timer timer;  // real-time timer
    cgroup_.attach(jailed_pid_);
    cgroup_.setMemoryLimit(conf_.memory_limit);
    cgroup_.setPidLimit(conf_.pid_limit);

    if (recvFrom_(SOCK::INSIDE) == MESSAGE::ERROR) {
        return;
    }

    timer.reset();
    cgroup_.resetTimer();
    startKiller_();
    sendTo_(SOCK::INSIDE, MESSAGE::RUN);
    RAW_DLOG(INFO, "waiting for jailed process");

    int status;
    if (waitpid(jailed_pid_, &status, 0) == -1) {
        RAW_LOG(ERROR, "failed to waitpid for jailed thread");
        throw std::runtime_error(strerror(errno));
    }
    RAW_DLOG(INFO, "jailed process exited");

    stopKiller_();

    RAW_DLOG(INFO, "starting to gather infomation");
    Result result;
    result.time.real = timer.tok();
    result.time.sys = cgroup_.getTimeSysUsage();
    result.time.usr = cgroup_.getTimeUsrUsage();
    result.memory = cgroup_.getMemoryUsage();
    if (WIFEXITED(status)) {
        result.return_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.signal = WTERMSIG(status);
    } else {
        RAW_LOG(ERROR, "jailed process exited for unknown reason");
        throw std::runtime_error("jailed process exited for unknown reason");
    }

    const auto &s = result.to_json().dump();
    writeToFd(STDOUT_FILENO, s.c_str(), s.length());
    return;
}

void Jail::setrlimits_() {
    rlimit64 time{
        .rlim_cur = static_cast<unsigned long>(conf_.cpu_time_limit.count()),
        .rlim_max = static_cast<unsigned long>(conf_.cpu_time_limit.count())};
    rlimit64 file{.rlim_cur = conf_.output_limit,
                  .rlim_max = conf_.output_limit};
    rlimit64 nfd{.rlim_cur = conf_.openfile_limit,
                 .rlim_max = conf_.openfile_limit};
    if (::setrlimit64(RLIMIT_CPU, &time) == -1 ||
        ::setrlimit64(RLIMIT_FSIZE, &file) == -1 ||
        ::setrlimit64(RLIMIT_OFILE, &nfd) == -1) {
        RAW_LOG(ERROR, "failed to setrlimit");
        throw std::runtime_error(strerror(errno));
    }
}

bool Jail::supervise_() {
    pollfd pollfds[2];
    pollfds[0] = {.fd = timer_fd_, .events = POLLIN, .revents = 0};
    pollfds[1] = {.fd = oom_notifier_fd_, .events = POLLIN, .revents = 0};
    int ret = TEMP_FAILURE_RETRY(
        poll(pollfds, 2, conf_.real_time_limit.count() * 1000));
    if (ret == -1) {
        RAW_LOG(ERROR, "failed to call poll");
        throw std::runtime_error(strerror(errno));
    }
    RAW_DLOG(INFO, "is disalarmed: %s, is timeout: %s, is oom: %s",
             (pollfds[0].revents & POLLIN ? "true" : "false"),
             (ret == 0 ? "true" : "false"),
             (pollfds[1].revents & POLLIN ? "true" : "false"));
    return !(pollfds[0].revents & POLLIN);
}

void Jail::startKiller_() {
    static const int stack_size = 128 * 1024;

    // _supervisor shoule be a thread of pid 1
    static const auto _supervisor = [](void *_jail) -> int {
        // full featured glog is not thread-safe
        auto jail = static_cast<Jail *>(_jail);
        RAW_DLOG(INFO, "supervisor started. counting down: %lds",
                 jail->conf_.real_time_limit.count());

        if (jail->supervise_()) {
            RAW_DLOG(INFO,
                     "resource usage exceed. trying to kill jailed process");
            if (!jail->killChild()) {
                RAW_LOG(ERROR, "failed to kill the jail. terminating...");
                return EXIT_FAILURE;
            }
        } /* else {
            RAW_DLOG(INFO, "killer disalarmed");
        } */
        return EXIT_SUCCESS;
    };

    uint8_t *stack =
        (uint8_t *)mmap(nullptr, stack_size, PROT_WRITE | PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == nullptr) {
        RAW_LOG(ERROR, "failed to call mmap");
        throw std::runtime_error(strerror(errno));
    }

    // _supervisor is detached. see man clone.2
    if (clone(_supervisor, stack + stack_size,
              CLONE_VM | CLONE_SIGHAND | CLONE_THREAD | CLONE_FILES,
              this) == -1) {
        RAW_LOG(ERROR, "filed to call clone");
        throw std::runtime_error(strerror(errno));
    }
    return;
}

void Jail::stopKiller_() {
    static const uint64_t disalarm{1};
    if (!writeToFd(timer_fd_, &disalarm, sizeof(disalarm))) {
        RAW_LOG(ERROR, "failed to disalarm killer");
        throw std::runtime_error(strerror(errno));
    }

    int status = 0;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        RAW_LOG(ERROR, "killer failed to kill jailed. terminating...");
        exit(EXIT_FAILURE);
    }
    return;
}

bool Jail::killChild() {
    // there is a chance jailed proc is already exited before we stop the killer
    // just ignore ESRCH
    if (kill(jailed_pid_, SIGKILL) == -1 && errno != ESRCH) {
        RAW_LOG(ERROR, "failed to kill jailed process: %s", strerror(errno));
        RAW_LOG(ERROR, "you should termiante everything right away");
        return false;
    }
    return true;
}

int Jail::run() {
    static const int stack_size = 16 * 1024 * 1024;
    uint8_t *stack =
        (uint8_t *)mmap(nullptr, stack_size, PROT_WRITE | PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == nullptr) {
        throw std::runtime_error(std::string("failed to call mmap: ") +
                                 strerror(errno));
    }

    jail_pid_ = clone(cloneWorkerProc_, stack + stack_size, CLONE_NEWPID, this);
    if (jail_pid_ == -1) {
        throw std::runtime_error(std::string("failed to call fork: ") +
                                 strerror(errno));
    }

    int status;
    if (waitpid(jail_pid_, &status, __WALL) == -1) {
        RAW_LOG(ERROR, "failed to waitpid for jail thread");
        throw std::runtime_error(strerror(errno));
    }
    if (!WIFEXITED(status)) {
        RAW_LOG(
            ERROR,
            "jail process exited unexceptedly: WIFSIGNALED: %d, WTERMSIG: %d",
            WIFSIGNALED(status), WTERMSIG(status));
        throw std::runtime_error("exited unexceptedly");
    }
    RAW_DLOG(INFO, "jail thread exited");

    return WEXITSTATUS(status);
}

void Jail::sendTo_(SOCK sock, MESSAGE stat) {
    auto fd = sock == SOCK::INSIDE ? sock_inside_ : sock_outside_;
    if (!writeToFd(fd, &stat, sizeof(MESSAGE))) {
        RAW_LOG(ERROR, "failed to write to socket");
        throw std::runtime_error(strerror(errno));
    }
}

Jail::MESSAGE Jail::recvFrom_(SOCK sock) {
    auto fd = sock == SOCK::INSIDE ? sock_inside_ : sock_outside_;
    Jail::MESSAGE stat;
    if (readFromFd(fd, &stat, sizeof(stat)) != sizeof(Jail::MESSAGE)) {
        RAW_LOG(ERROR, "failed to read from socket");
        throw std::runtime_error(strerror(errno));
    }
    return stat;
}

Jail::~Jail() {
    close(sock_inside_);
    close(sock_outside_);
    close(timer_fd_);
    close(oom_notifier_fd_);
}

}  // namespace yamc
