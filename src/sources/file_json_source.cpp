#include "crossbring/sources/file_json_source.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace crossbring {

void FileJsonSource::start() {
    if (running_.exchange(true)) return;
    th_ = std::thread([this]{ run(); });
}

void FileJsonSource::stop() {
    if (!running_.exchange(false)) return;
    if (th_.joinable()) th_.join();
}

void FileJsonSource::run() {
    spdlog::info("FileJsonSource watching {}", file_.string());
    while (running_.load()) {
        try {
            load_once();
        } catch (const std::exception& e) {
            spdlog::warn("FileJsonSource error: {}", e.what());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

static std::string file_fingerprint(const std::filesystem::path& p) {
    std::error_code ec;
    auto sz = std::filesystem::file_size(p, ec);
    auto wt = std::filesystem::last_write_time(p, ec).time_since_epoch().count();
    return std::to_string(sz) + ":" + std::to_string(wt);
}

bool FileJsonSource::load_once() {
    if (!std::filesystem::exists(file_)) return false;
    std::string fp = file_fingerprint(file_);
    if (fp == last_fingerprint_) return false; // unchanged
    last_fingerprint_ = fp;

    std::ifstream in(file_);
    if (!in) return false;
    nlohmann::json j;
    in >> j;

    // If content has top-level 'ads' like AF response
    nlohmann::json arr;
    if (j.is_object() && j.contains("ads") && j["ads"].is_array()) {
        arr = j["ads"];
    } else if (j.is_array()) {
        arr = j;
    } else {
        spdlog::warn("FileJsonSource: JSON not array or ads[]");
        return false;
    }

    size_t emitted = 0;
    for (auto& item : arr) {
        std::string key;
        if (item.contains("id")) key = item["id"].get<std::string>();
        else if (item.contains("job_id")) key = item["job_id"].get<std::string>();
        else key = std::to_string(emitted);

        Event ev;
        ev.tp = std::chrono::steady_clock::now();
        ev.source = source_name_;
        ev.key = key;
        ev.payload = item;
        engine_.submit(std::move(ev));
        ++emitted;
    }
    spdlog::info("FileJsonSource: emitted {} event(s) from {}", emitted, file_.string());
    return emitted > 0;
}

} // namespace crossbring

