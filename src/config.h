#ifndef CONFIG_H_
#define CONFIG_H_

#include <chrono>

#include "common.h"

namespace yamc {

struct Config {
    inline static const mount_list_t default_robind{
        {"/bin", "/bin"},         {"/lib", "/lib"},
        {"/lib64", "/lib64"},     {"/usr/bin", "/usr/bin"},
        {"/usr/lib", "/usr/lib"}, {"/usr/lib64", "/usr/lib64"},
    };
    inline static const mount_list_t default_rwbind{
        {"/dev/null", "/dev/null"},
        {"/dev/zero", "/dev/zero"},
        {"/dev/random", "/dev/random"},
        {"/dev/urandom", "/dev/urandom"},
    };
    inline static const std::vector<std::string> default_env{
        "PATH=/bin:/usr/bin"};

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
    UTSConfig uts;
    IDMap use_uid;
    IDMap use_gid;
    mount_list_t rwbind;
    mount_list_t robind;
    std::vector<std::string> env;

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