#ifndef CGROUP_H_
#define CGROUP_H_

#include "common.h"

namespace yamc {

class Cgroup {
   private:
    enum class CG_SUBSYS { MEMORY, CPU, CPUACCT, PIDS };
    inline static std::filesystem::path baseDir_ = "/sys/fs/cgroup";
    const std::filesystem::path &getSubsysPath_(CG_SUBSYS subsys) const;

    std::string name_;

    template <typename T>
    T readFrom_(CG_SUBSYS subsys, const std::string &filename) const;

    template <typename T>
    void writeTo_(CG_SUBSYS subsys, const std::string &filename,
                  const T &buf) const;

   public:
    Cgroup();
    Cgroup(Cgroup const &) = delete;
    Cgroup &operator=(Cgroup const &) = delete;

    void attach(pid_t pid) const;

    void resetTimer() const;

    /**
     * @brief Get the time usage (user) in nanoseconds
     */
    long long getTimeUsrUsage() const;

    /**
     * @brief Get the time usage (sys) in nanoseconds
     */
    long long getTimeSysUsage() const;

    // https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/resource_management_guide/ch-subsystems_and_tunable_parameters#blkio-throttling
    /**
     * @brief Get the memory usage in bytes
     */
    long long getMemoryUsage() const;

    /**
     * @brief set memory limit in bytes
     *
     */
    void setMemoryLimit(long long limit_bytes) const;

    void setPidLimit(int pids) const;

    void regOOMNotifier(int eventfd) const;

    ~Cgroup();
};

}  // namespace yamc

#endif  // CGROUP_H_