#pragma once

#include <deque>
#include <mutex>
#include <nlohmann/json.hpp>

#include "crossbring/sinks/sink.h"

namespace crossbring {

class RecentBuffer {
public:
    explicit RecentBuffer(size_t capacity) : capacity_(capacity) {}

    void push(nlohmann::json item) {
        std::lock_guard<std::mutex> lock(mu_);
        if (buf_.size() >= capacity_) buf_.pop_front();
        buf_.push_back(std::move(item));
    }

    nlohmann::json snapshot_json(size_t max_items = 0) {
        std::lock_guard<std::mutex> lock(mu_);
        nlohmann::json arr = nlohmann::json::array();
        size_t start = 0;
        if (max_items > 0 && buf_.size() > max_items) start = buf_.size() - max_items;
        for (size_t i = start; i < buf_.size(); ++i) arr.push_back(buf_[i]);
        return arr;
    }

private:
    size_t capacity_;
    std::deque<nlohmann::json> buf_;
    std::mutex mu_;
};

class RecentBufferSink : public Sink {
public:
    explicit RecentBufferSink(std::shared_ptr<RecentBuffer> buf) : buf_(std::move(buf)) {}
    void consume(const Event& ev) override {
        nlohmann::json j = {
            {"source", ev.source},
            {"key", ev.key},
            {"payload", ev.payload}
        };
        buf_->push(std::move(j));
    }
    std::string name() const override { return "recent_buffer"; }

private:
    std::shared_ptr<RecentBuffer> buf_;
};

} // namespace crossbring

