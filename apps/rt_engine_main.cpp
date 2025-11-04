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
#include "crossbring/sinks/batching_sink.h"
#include "crossbring/sinks/recent_buffer_sink.h"
#include "crossbring/http/http_server.h"

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
    std::string backpressure = cfg.value("backpressure", std::string("block"));
    bool drop_on_full = (backpressure == "drop");
    Engine engine(queue_cap, workers, drop_on_full);

    // Example processor: add ingest_ts to payload
    engine.add_processor([](Event& ev){
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ev.tp.time_since_epoch()).count();
        ev.payload["ingest_ts_ns"] = ns;
    });

    // Sinks
    auto add_sink = [&](std::shared_ptr<Sink> s)->std::shared_ptr<Sink>{
        // Optional batching wrapper
        if (cfg["sinks"].contains("batching") && cfg["sinks"]["batching"].value("enabled", false)) {
            size_t bs = cfg["sinks"]["batching"].value("batch_size", 32);
            int fm = cfg["sinks"]["batching"].value("flush_ms", 200);
            s = std::make_shared<BatchingSink>(s, bs, fm);
        }
        engine.add_sink(s);
        return s;
    };

    if (cfg["sinks"].value("console", true)) {
        add_sink(make_console_sink());
    }
#ifdef USE_SQLITE
    if (cfg["sinks"].contains("sqlite") && cfg["sinks"]["sqlite"].value("enabled", false)) {
        auto path = cfg["sinks"]["sqlite"].value("path", std::string("data/events.sqlite"));
        try {
            add_sink(make_sqlite_sink(path));
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

    // Recent buffer + HTTP server
    std::shared_ptr<RecentBuffer> recent;
#ifdef USE_HTTP_SERVER
    if (cfg.contains("http") && cfg["http"].value("enabled", true)) {
        recent = std::make_shared<RecentBuffer>(cfg["http"].value("recent_capacity", 500));
        add_sink(std::make_shared<RecentBufferSink>(recent));
    }
#endif

    engine.start();
    for (auto& s : sensors) s->start();
    for (auto& f : files) f->start();

    std::signal(SIGINT, on_sigint);
#ifdef SIGTERM
    std::signal(SIGTERM, on_sigint);
#endif

    std::unique_ptr<HttpServer> http;
#ifdef USE_HTTP_SERVER
    if (recent) {
        std::string host = cfg["http"].value("host", std::string("127.0.0.1"));
        int port = cfg["http"].value("port", 9100);
        http = std::make_unique<HttpServer>(engine, recent, host, port);
        http->start();
        spdlog::info("HTTP server on http://{}:{}/", host, port);
    }
#endif

    spdlog::info("Engine running. Press Ctrl+C to stop.");
    while (!g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    for (auto& f : files) f->stop();
    for (auto& s : sensors) s->stop();
    if (http) http->stop();
    engine.stop();
    spdlog::info("Shutdown complete. processed={} dropped={}", engine.processed_count(), engine.dropped_count());
    return 0;
}
