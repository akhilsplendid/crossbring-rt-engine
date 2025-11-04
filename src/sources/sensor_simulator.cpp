#include "crossbring/sources/sensor_simulator.h"

#include <chrono>
#include <random>
#include <thread>

namespace crossbring {

void SensorSimulator::start() {
    if (running_.exchange(true)) return;
    th_ = std::thread([this]{ run(); });
}

void SensorSimulator::stop() {
    if (!running_.exchange(false)) return;
    if (th_.joinable()) th_.join();
}

void SensorSimulator::run() {
    std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<double> dist(50.0, 10.0);
    while (running_.load()) {
        Event ev;
        ev.tp = std::chrono::steady_clock::now();
        ev.source = sensor_name_;
        ev.key = "sensor-" + sensor_name_;
        ev.payload = {
            {"type", "sensor"},
            {"name", sensor_name_},
            {"value", dist(rng)}
        };
        engine_.submit(std::move(ev));
        std::this_thread::sleep_for(std::chrono::milliseconds(period_ms_));
    }
}

} // namespace crossbring

