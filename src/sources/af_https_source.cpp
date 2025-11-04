#ifdef USE_CPR

#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

#include "crossbring/core/engine.h"

namespace crossbring {

class AfHttpsSource {
public:
    AfHttpsSource(Engine& engine, std::string source_name, std::string query_payload, int interval_ms)
        : engine_(engine), source_(std::move(source_name)), payload_(std::move(query_payload)), interval_ms_(interval_ms) {}

    void start() { running_ = true; th_ = std::thread([this]{ run(); }); }
    void stop() { running_ = false; if (th_.joinable()) th_.join(); }

private:
    void run() {
        const std::string url = "https://platsbanken-api.arbetsformedlingen.se/jobs/v1/search";
        while (running_) {
            try {
                auto r = cpr::Post(cpr::Url{url}, cpr::Header{{"Content-Type","application/json"}}, cpr::Body{payload_});
                if (r.status_code == 200) {
                    auto j = nlohmann::json::parse(r.text);
                    if (j.contains("ads") && j["ads"].is_array()) {
                        for (auto& item : j["ads"]) {
                            Event ev; ev.tp = std::chrono::steady_clock::now(); ev.source = source_; ev.key = item.value("id", ""); ev.payload = item; engine_.submit(std::move(ev));
                        }
                        spdlog::info("AF HTTPS emitted {} ad(s)", j["ads"].size());
                    }
                } else {
                    spdlog::warn("AF HTTPS status {}", r.status_code);
                }
            } catch (const std::exception& e) {
                spdlog::warn("AF HTTPS error: {}", e.what());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
        }
    }

    Engine& engine_;
    std::string source_;
    std::string payload_;
    int interval_ms_;
    std::atomic<bool> running_{false};
    std::thread th_;
};

} // namespace crossbring

#endif // USE_CPR

