#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "crossbring/core/engine.h"
#include "crossbring/sources/sensor_simulator.h"
#include "crossbring/sources/file_json_source.h"
#include "crossbring/sinks/console_sink.h"
#include "crossbring/sinks/sqlite_sink.h"

using namespace crossbring;

static std::atomic<bool> g_stop{false};

void on_sigint(int) { g_stop = true; }

int main(int argc, char** argv) {
    std::filesystem::path configPath = argc > 1 ? argv[1] : std::filesystem::path("configs/config.example.json");
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::info("Crossbring RT Engine starting. Config: {}", configPath.string());

    nlohmann::json cfg;
    try {
        std::ifstream in(configPath);
        if (!in) {
            spdlog::error("Cannot open config file: {}", configPath.string());
            return 1;
        }
        in >> cfg;
    } catch (const std::exception& e) {
        spdlog::error("Config parse error: {}", e.what());
        return 2;
    }

    size_t queue_cap = cfg.value("queue_capacity", 1024);
    size_t workers = cfg.value("workers", std::thread::hardware_concurrency());
    Engine engine(queue_cap, workers);

    // Example processor: add ingest_ts to payload
    engine.add_processor([](Event& ev){
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ev.tp.time_since_epoch()).count();
        ev.payload["ingest_ts_ns"] = ns;
    });

    // Sinks
    if (cfg["sinks"].value("console", true)) {
        engine.add_sink(make_console_sink());
    }
#ifdef USE_SQLITE
    if (cfg["sinks"].contains("sqlite") && cfg["sinks"]["sqlite"].value("enabled", false)) {
        auto path = cfg["sinks"]["sqlite"].value("path", std::string("data/events.sqlite"));
        try {
            engine.add_sink(make_sqlite_sink(path));
            spdlog::info("SQLite sink enabled at {}", path);
        } catch (const std::exception& e) {
            spdlog::warn("SQLite sink failed to initialize: {}", e.what());
        }
    }
#endif

    // Sources
    std::vector<std::unique_ptr<SensorSimulator>> sensors;
    if (cfg["sources"].contains("sensors")) {
        for (auto& s : cfg["sources"]["sensors"]) {
            auto name = s.value("name", std::string("sensor"));
            int period = s.value("period_ms", 50);
            sensors.emplace_back(std::make_unique<SensorSimulator>(engine, name, period));
        }
    }

    std::vector<std::unique_ptr<FileJsonSource>> files;
    if (cfg["sources"].contains("file_json")) {
        for (auto& f : cfg["sources"]["file_json"]) {
            auto src = f.value("source", std::string("file_json"));
            auto path = f.value("path", std::string("data/af_jobs.json"));
            int interval = f.value("interval_ms", 1000);
            files.emplace_back(std::make_unique<FileJsonSource>(engine, src, path, interval));
        }
    }

    engine.start();
    for (auto& s : sensors) s->start();
    for (auto& f : files) f->start();

    std::signal(SIGINT, on_sigint);
#ifdef SIGTERM
    std::signal(SIGTERM, on_sigint);
#endif

    spdlog::info("Engine running. Press Ctrl+C to stop.");
    while (!g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    for (auto& f : files) f->stop();
    for (auto& s : sensors) s->stop();
    engine.stop();
    spdlog::info("Shutdown complete. processed={} dropped={}", engine.processed_count(), engine.dropped_count());
    return 0;
}

