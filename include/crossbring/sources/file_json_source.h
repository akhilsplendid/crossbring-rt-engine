#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include "crossbring/core/engine.h"

namespace crossbring {

// Polls a JSON file containing an array of objects and emits events for each
class FileJsonSource {
public:
    FileJsonSource(Engine& engine, std::string source_name, std::filesystem::path file, int interval_ms = 1000)
        : engine_(engine), source_name_(std::move(source_name)), file_(std::move(file)), interval_ms_(interval_ms) {}

    void start();
    void stop();

private:
    void run();
    bool load_once();

    Engine& engine_;
    std::string source_name_;
    std::filesystem::path file_;
    int interval_ms_;
    std::atomic<bool> running_{false};
    std::thread th_;
    std::string last_fingerprint_;
};

} // namespace crossbring

