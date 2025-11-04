#pragma once

#include <atomic>
#include <random>
#include <string>
#include <thread>

#include "crossbring/core/engine.h"

namespace crossbring {

class SensorSimulator {
public:
    SensorSimulator(Engine& engine, std::string sensor_name, int period_ms = 50)
        : engine_(engine), sensor_name_(std::move(sensor_name)), period_ms_(period_ms) {}

    void start();
    void stop();

private:
    void run();

    Engine& engine_;
    std::string sensor_name_;
    int period_ms_;
    std::atomic<bool> running_{false};
    std::thread th_;
};

} // namespace crossbring

