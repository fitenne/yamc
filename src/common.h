#ifndef COMMON_H_
#define COMMON_H_

#define UNUSED(x) (void)(x)

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace yamc {
namespace fs = std::filesystem;

struct MountPt {
    enum class MNT_TYPE { ROBIND, RWBIND, TMPFS };
    fs::path src;
    fs::path dest;
    std::string option;
    MNT_TYPE type;
    MountPt(const fs::path &src, const fs::path &dest,
            const std::string &option, const MNT_TYPE type);
};
using mount_list_t = std::vector<MountPt>;

struct Symlink {
    fs::path src;
    fs::path dest;
    Symlink(const fs::path &src, const fs::path &dest);
};
using symlink_list_t = std::vector<Symlink>;

struct IDMap {
    typedef unsigned int id_t;
    static const id_t NOID = (id_t)(-1);

    id_t inside_id = 0;
    id_t outside_id = NOID;
    unsigned count = 1;
    IDMap(id_t in = 0, id_t out = NOID, unsigned count = 1);
};

struct TimeUsage {
    long long sys = 0;
    long long usr = 0;
    long long real = 0;
};

struct Result {
    TimeUsage time{};
    long long memory = 0;
    int return_code = 0;
    int signal = 0;

    nlohmann::json to_json() const;
};

struct UTSConfig {
    std::string hostname = "yamc";
    std::string domainname = "yamc";
};

}  // namespace yamc

#endif  // COMMON_H_