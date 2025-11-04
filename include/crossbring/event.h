#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace crossbring {

struct Event {
    std::chrono::steady_clock::time_point tp;
    std::string source;   // e.g., sensor name or "af_jobs"
    std::string key;      // e.g., sensor id or job id
    nlohmann::json payload; // raw data
};

} // namespace crossbring

