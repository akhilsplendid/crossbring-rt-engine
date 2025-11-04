#pragma once

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <nlohmann/json.hpp>

#include "crossbring/sinks/sink.h"

namespace crossbring {

class JsonQueue {
public:
    void push(nlohmann::json j) {
        std::lock_guard<std::mutex> lock(mu_);
        q_.push_back(std::move(j));
        cv_.notify_one();
    }
    bool pop(nlohmann::json& out) {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [&]{ return stop_ || !q_.empty(); });
        if (stop_ && q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop_front();
        return true;
    }
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            stop_ = true;
        }
        cv_.notify_all();
    }
private:
    std::deque<nlohmann::json> q_;
    std::mutex mu_;
    std::condition_variable cv_;
    bool stop_ = false;
};

class EventHub {
public:
    std::shared_ptr<JsonQueue> register_consumer() {
        auto q = std::make_shared<JsonQueue>();
        std::lock_guard<std::mutex> lock(mu_);
        consumers_.push_back(q);
        return q;
    }
    void publish(const nlohmann::json& j) {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto it = consumers_.begin(); it != consumers_.end();) {
            if (auto q = it->lock()) { q->push(j); ++it; }
            else { it = consumers_.erase(it); }
        }
    }
private:
    std::vector<std::weak_ptr<JsonQueue>> consumers_;
    std::mutex mu_;
};

class EventHubSink : public Sink {
public:
    explicit EventHubSink(std::shared_ptr<EventHub> hub) : hub_(std::move(hub)) {}
    void consume(const Event& ev) override {
        nlohmann::json j = {
            {"source", ev.source},
            {"key", ev.key},
            {"payload", ev.payload}
        };
        hub_->publish(j);
    }
    std::string name() const override { return "event_hub"; }
private:
    std::shared_ptr<EventHub> hub_;
};

} // namespace crossbring

