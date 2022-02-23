#include "cgroup.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <random>

#include "utils.h"

namespace yamc {

Cgroup::Cgroup() {
    static std::random_device rd{};
    static std::mt19937_64 rd64(rd());
    name_ = std::string("yamc") + std::to_string(rd64());
    RAW_DLOG(INFO, "cgroup name is %s", name_.c_str());
    fs::create_directory(getSubsysPath_(CG_SUBSYS::CPU));
    fs::create_directory(getSubsysPath_(CG_SUBSYS::CPUACCT));
    fs::create_directory(getSubsysPath_(CG_SUBSYS::MEMORY));
    // fs::create_directory(getSubsysPath_(CG_SUBSYS::BLKIO));
    fs::create_directory(getSubsysPath_(CG_SUBSYS::PIDS));
}

template <typename T>
T Cgroup::readFrom_(CG_SUBSYS subsys, const std::string &filename) const {
    T ret;
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(getSubsysPath_(subsys) / filename);
    ifs >> ret;
    return ret;
}

template <typename T>
void Cgroup::writeTo_(CG_SUBSYS subsys, const std::string &filename,
                      const T &buf) const {
    std::ofstream ofs;
    ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ofs.open(getSubsysPath_(subsys) / filename);
    ofs << buf;
    return;
}

void Cgroup::attach(pid_t pid) const {
    writeTo_(CG_SUBSYS::CPU, "cgroup.procs", pid);
    writeTo_(CG_SUBSYS::CPUACCT, "cgroup.procs", pid);
    writeTo_(CG_SUBSYS::MEMORY, "cgroup.procs", pid);
    writeTo_(CG_SUBSYS::PIDS, "cgroup.procs", pid);
}

const std::filesystem::path &Cgroup::getSubsysPath_(CG_SUBSYS subsys) const {
    namespace fs = std::filesystem;
    std::filesystem::path *ret = nullptr;
    switch (subsys) {
        case CG_SUBSYS::CPU:
            static fs::path cpu = baseDir_ / "cpu" / "yamc" / name_;
            ret = &cpu;
            break;
        case CG_SUBSYS::CPUACCT:
            static fs::path cpuacct = baseDir_ / "cpuacct" / "yamc" / name_;
            ret = &cpuacct;
            break;
        case CG_SUBSYS::MEMORY:
            static fs::path memory = baseDir_ / "memory" / "yamc" / name_;
            ret = &memory;
            break;
        case CG_SUBSYS::PIDS:
            static fs::path pids = baseDir_ / "pids" / "yamc" / name_;
            ret = &pids;
            break;
        default:
            throw std::runtime_error("unknown cgroup subsystem");
            break;
    }
    return *ret;
}

void Cgroup::resetTimer() const {
    writeTo_(CG_SUBSYS::CPUACCT, "cpuacct.usage", std::string("0"));
}

long long Cgroup::getTimeUsrUsage() const {
    return readFrom_<long long>(CG_SUBSYS::CPUACCT, "cpuacct.usage_user");
}

long long Cgroup::getTimeSysUsage() const {
    return readFrom_<long long>(CG_SUBSYS::CPUACCT, "cpuacct.usage_sys");
}

long long Cgroup::getMemoryUsage() const {
    return readFrom_<long long>(CG_SUBSYS::MEMORY,
                                "memory.memsw.max_usage_in_bytes");
}

void Cgroup::setMemoryLimit(long long limit_bytes) const {
    RAW_DLOG(INFO, "setting memory limit: %lld", limit_bytes);
    writeTo_(CG_SUBSYS::MEMORY, "memory.swappiness", 0);
    writeTo_(CG_SUBSYS::MEMORY, "memory.limit_in_bytes", limit_bytes);
    writeTo_(CG_SUBSYS::MEMORY, "memory.memsw.limit_in_bytes", limit_bytes);
    return;
}

void Cgroup::setPidLimit(int pids) const {
    writeTo_(CG_SUBSYS::PIDS, "pids.max", pids);
}

void Cgroup::regOOMNotifier(int eventfd) const {
    // write "1" to memory.oom_control to disable oom-killer
    // see 10. OOM Control at
    // https://www.kernel.org/doc/Documentation/cgroup-v1/memory.txt
    static const char enable_oom_killer[] = "1";

    char buf[128];
    const auto oom_control_path =
        getSubsysPath_(CG_SUBSYS::MEMORY) / "memory.oom_control";
    auto oom_control_fd = open(oom_control_path.c_str(), O_WRONLY);
    if (oom_control_fd == -1) {
        throw std::runtime_error(strerror(errno));
    }
    int bs = snprintf(buf, sizeof(buf), "%d %d", eventfd, oom_control_fd) + 1;

    if (!writeToFd(oom_control_fd, enable_oom_killer,
                   sizeof(enable_oom_killer)) ||
        !writeBufToFile(
            getSubsysPath_(CG_SUBSYS::MEMORY) / "cgroup.event_control", buf,
            bs)) {
        close(oom_control_fd);
        throw std::runtime_error(strerror(errno));
    }
    close(oom_control_fd);
}

Cgroup::~Cgroup() {
    try {
        fs::remove(getSubsysPath_(CG_SUBSYS::CPUACCT));
        fs::remove(getSubsysPath_(CG_SUBSYS::CPU));
        fs::remove(getSubsysPath_(CG_SUBSYS::MEMORY));
        // fs::remove(getSubsysPath_(CG_SUBSYS::BLKIO));
        fs::remove(getSubsysPath_(CG_SUBSYS::PIDS));
    } catch (const std::exception &e) {
        RAW_LOG(ERROR, "failed to remove cgroup dir: %s", e.what());
    }
}

}  // namespace yamc
