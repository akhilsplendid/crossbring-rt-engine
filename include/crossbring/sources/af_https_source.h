#pragma once

#ifdef USE_CPR

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "crossbring/core/engine.h"

namespace crossbring {

class AfHttpsSource {
public:
    AfHttpsSource(Engine& engine, std::string source_name, std::string query_payload, int interval_ms);
    void start();
    void stop();

private:
    void run();

    Engine& engine_;
    std::string source_;
    std::string payload_;
    int interval_ms_;
    std::atomic<bool> running_{false};
    std::thread th_;
};

} // namespace crossbring

#endif // USE_CPR

