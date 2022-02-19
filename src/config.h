#ifndef CONFIG_H_
#define CONFIG_H_

#include <chrono>

#include "common.h"

namespace yamc {

struct Config {
    inline static const mount_list_t default_robind{
        {"/bin", "/bin", "", MountPt::MNT_TYPE::ROBIND},
        {"/lib", "/lib", "", MountPt::MNT_TYPE::ROBIND},
        {"/lib64", "/lib64", "", MountPt::MNT_TYPE::ROBIND},
        {"/usr/bin", "/usr/bin", "", MountPt::MNT_TYPE::ROBIND},
        {"/usr/lib", "/usr/lib", "", MountPt::MNT_TYPE::ROBIND},
        {"/usr/lib64", "/usr/lib64", "", MountPt::MNT_TYPE::ROBIND},
    };
    inline static const mount_list_t default_rwbind{
        {"/dev/null", "/dev/null", "", MountPt::MNT_TYPE::RWBIND},
        {"/dev/zero", "/dev/zero", "", MountPt::MNT_TYPE::RWBIND},
        {"/dev/random", "/dev/random", "", MountPt::MNT_TYPE::RWBIND},
        {"/dev/urandom", "/dev/urandom", "", MountPt::MNT_TYPE::RWBIND},
    };
    inline static const symlink_list_t default_symlink{
        {"/dev/stdin", "/proc/self/fd/0"},
        {"/dev/stdout", "/proc/self/fd/1"},
        {"/dev/stderr", "/proc/self/fd/2"},
    };
    inline static const mount_list_t default_tmpfs{
        {"", "/run", "mode=755,size=16777216", MountPt::MNT_TYPE::TMPFS},
        {"", "/tmp", "mode=777,size=16777216", MountPt::MNT_TYPE::TMPFS},
    };
    inline static const std::vector<std::string> default_env{
        "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:.",
    };

    /*
     * resource limit
     */
    std::chrono::seconds cpu_time_limit = std::chrono::seconds(3);
    std::chrono::seconds real_time_limit = std::chrono::seconds(10);
    unsigned long memory_limit = 32 * 1024 * 1024;  // bytes
    unsigned long output_limit = 10 * 1024 * 1024;  // bytes
    unsigned long pid_limit = 16;
    unsigned long openfile_limit = 16;

    /*
     * container config
     */
    UTSConfig uts = UTSConfig{};
    IDMap use_uid = IDMap{};
    IDMap use_gid = IDMap{};
    mount_list_t rwbind = default_rwbind;
    mount_list_t robind = default_robind;
    mount_list_t tmpfs = default_tmpfs;
    symlink_list_t symlink = default_symlink;
    std::vector<std::string> env = default_env;

    /*
     * spawn options
     */
    // https://www.gnu.org/software/libc/manual/html_node/Program-Arguments.html
    std::vector<std::string> cmdline;
    fs::path chroot_path;       // chroot path, defaults to /tmp/yamc<pid>
    fs::path chdir_path = "/";  // chdir after chroot
    int stdout_fd = -1;         // redirect stdout to this fd
};

Config parseOptions(int argc, char* argv[]);

}  // namespace yamc

#endif  // CONFIG_H_