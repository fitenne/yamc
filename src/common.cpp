#include "common.h"

#include <cstring>

namespace yamc {

IDMap::IDMap(id_t in, id_t out, unsigned count)
    : inside_id(in), outside_id(out), count(count) {}

nlohmann::json Result::to_json() const {
    nlohmann::json j;
    j["time"]["sys"] = time.sys;
    j["time"]["usr"] = time.usr;
    j["time"]["real"] = time.real;
    j["memory"] = memory;
    j["returnCode"] = return_code;
    j["signal"] = signal;
    return j;
}

}  // namespace yamc