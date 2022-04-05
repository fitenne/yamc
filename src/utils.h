#ifndef UTILS_H_
#define UTILS_H_

#include "common.h"

namespace yamc {

int pivot_root(const char* new_root, const char* put_old);

int tgkill(int tgid, int tid, int sig);

pid_t gettid();

void mountFs(const MountPt& mnt, const fs::path& chroot,
             unsigned long addtional_flag);

/**
 * @brief get a vector of pointers to c-string from a vector of std::string
 * with a null pointer appended to the end. caller is responsible for the
 * valiadation of the pointers
 *
 * @return std::vector<const char*> pointers to data of std::string
 */
std::vector<const char*> strvec2cstr(const std::vector<std::string>& args);

/**
 * @brief fork and exec
 *
 * @param args cmdline
 * @return pid of child process
 */
pid_t systemExec(const std::vector<std::string> args);

void moveToNS(const fs::path& path);

ssize_t readFromFd(int fd, void* buf, size_t len);

bool writeToFd(int fd, const void* buf, size_t len);

bool writeBufToFile(const fs::path& filename, const void* buf, size_t len);

}  // namespace yamc

#endif  // UTILS_H_