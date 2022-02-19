#include "utils.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace yamc {
namespace fs = std::filesystem;

int pivot_root(const char *new_root, const char *put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

int tgkill(int tgid, int tid, int sig) {
    return syscall(SYS_tgkill, tgid, tid, sig);
}

pid_t gettid() { return syscall(SYS_gettid); }

// /**
//  * @brief recursively bind mount src to target with addtional_flag
//  *
//  * @param addtional_flag see man mount.2
//  */
// void bindMount(const fs::path &src, const fs::path &target,
//                unsigned long addtional_flag) {
//     if (fs::is_directory(src) && !fs::exists(target)) {
//         fs::create_directories(target);
//     } else {
//         const auto &par = target.parent_path();
//         if (par == target) {
//             // root provided as target?
//             throw std::runtime_error("unexcepted mount point");
//         }
//         if (!fs::exists(par)) {
//             fs::create_directories(par);
//         }
//         int fd = creat(target.c_str(), 0644);
//         if (fd == -1) {
//             throw std::runtime_error("failed to create " + target.string() +
//                                      ": " + strerror(errno));
//         }
//         close(fd);
//     }

//     if (mount(src.c_str(), target.c_str(), "", MS_BIND | MS_REC, "") == -1 ||
//         mount("", target.c_str(), "",
//               MS_REMOUNT | MS_BIND | MS_REC | MS_NOSUID | addtional_flag,
//               "") == -1) {
//         throw std::runtime_error(std::string("failed to remount : ") +
//                                  strerror(errno));
//     }
// }

void mountFs(const MountPt &mnt, const fs::path &chroot,
             unsigned long addtional_flag) {
    const auto target = chroot / mnt.dest.lexically_relative("/");

    if (mnt.type == MountPt::MNT_TYPE::TMPFS) {
        fs::create_directories(target);
        if (mount("", target.c_str(), "tmpfs", MS_NODEV | MS_NOEXEC | MS_NOSUID,
                  mnt.option.c_str()) == -1) {
            throw std::runtime_error(std::string("failed to mount tmpfs: ") +
                                     strerror(errno));
        }
    } else {
        if (fs::is_directory(mnt.src) && !fs::exists(target)) {
            fs::create_directories(target);
        } else {
            const auto &par = target.parent_path();
            // root provided as target?
            if (par == target)
                throw std::runtime_error("unexcepted mount point");
            if (!fs::exists(par)) {
                fs::create_directories(par);
            }
            int fd = creat(target.c_str(), 0644);
            if (fd == -1) {
                throw std::runtime_error("failed to create " + target.string() +
                                         ": " + strerror(errno));
            }
            close(fd);
        }

        if (mnt.type == MountPt::MNT_TYPE::ROBIND) {
            if (mount(mnt.src.c_str(), target.c_str(), "", MS_BIND | MS_REC,
                      "") == -1 ||
                mount(
                    "", target.c_str(), "",
                    MS_REMOUNT | MS_BIND | MS_REC | MS_RDONLY | addtional_flag,
                    "") == -1) {
                throw std::runtime_error(std::string("failed to remount : ") +
                                         strerror(errno));
            }
        } else {
            if (mount(mnt.src.c_str(), target.c_str(), "", MS_BIND | MS_REC,
                      "") == -1 ||
                mount("", target.c_str(), "",
                      MS_REMOUNT | MS_BIND | MS_REC | addtional_flag,
                      "") == -1) {
                throw std::runtime_error(std::string("failed to remount : ") +
                                         strerror(errno));
            }
        }
    }
}

std::vector<const char *> strvec2cstr(const std::vector<std::string> &args) {
    std::vector<const char *> helper(args.size());
    std::transform(args.begin(), args.end(), helper.begin(),
                   [](const std::string &s) { return s.data(); });
    helper.emplace_back(nullptr);
    return helper;
}

pid_t systemExec(const std::vector<std::string> args) {
    auto helper = strvec2cstr(args);
    auto pid = fork();
    if (pid == -1) {
        throw std::runtime_error(strerror(errno));
    }

    if (pid == 0) {
        execv(helper[0], (char *const *)helper.data());
        // unreachable code
        exit(EXIT_FAILURE);
    }
    return pid;
}

bool writeToFd(int fd, const void *buf, size_t len) {
    const uint8_t *charbuf = (const uint8_t *)buf;

    size_t writtenSz = 0;
    while (writtenSz < len) {
        ssize_t sz =
            TEMP_FAILURE_RETRY(write(fd, &charbuf[writtenSz], len - writtenSz));
        if (sz < 0) {
            return false;
        }
        writtenSz += sz;
    }
    return true;
}

bool writeBufToFile(const fs::path &filename, const void *buf, size_t len) {
    int fd;
    TEMP_FAILURE_RETRY(fd = open(filename.c_str(), O_WRONLY, 0644));
    if (fd == -1) {
        throw std::runtime_error(strerror(errno));
    }

    if (!writeToFd(fd, buf, len)) {
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

ssize_t readFromFd(int fd, void *buf, size_t len) {
    uint8_t *charbuf = (uint8_t *)buf;

    size_t readSz = 0;
    while (readSz < len) {
        ssize_t sz =
            TEMP_FAILURE_RETRY(read(fd, &charbuf[readSz], len - readSz));
        if (sz <= 0) {
            break;
        }
        readSz += sz;
    }
    return readSz;
}

void moveToNS(const fs::path &path) {
    int userns = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (setns(userns, 0) == -1) {
        close(userns);
        throw std::runtime_error(strerror(errno));
    }
    close(userns);
}

}  // namespace yamc
