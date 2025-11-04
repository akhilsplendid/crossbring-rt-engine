#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "crossbring/sinks/sink.h"

namespace crossbring {

class BatchingSink : public Sink {
public:
    BatchingSink(std::shared_ptr<Sink> inner, size_t batch_size, int flush_ms)
        : inner_(std::move(inner)), batch_size_(batch_size), flush_ms_(flush_ms) {
        running_ = true;
        th_ = std::thread([this]{ loop(); });
    }
    ~BatchingSink() override {
        {
            std::lock_guard<std::mutex> lock(mu_);
            running_ = false;
        }
        cv_.notify_all();
        if (th_.joinable()) th_.join();
        flush();
    }

    void consume(const Event& ev) override {
        std::lock_guard<std::mutex> lock(mu_);
        buf_.push_back(ev);
        if (buf_.size() >= batch_size_) cv_.notify_all();
    }

    std::string name() const override { return std::string("batch(") + inner_->name() + ")"; }

private:
    void loop() {
        std::unique_lock<std::mutex> lock(mu_);
        while (running_) {
            cv_.wait_for(lock, std::chrono::milliseconds(flush_ms_));
            flush_unlocked();
        }
    }

    void flush() {
        std::lock_guard<std::mutex> lock(mu_);
        flush_unlocked();
    }

    void flush_unlocked() {
        if (buf_.empty()) return;
        auto items = std::move(buf_);
        buf_.clear();
        // unlock before forwarding to avoid blocking producers
        mu_.unlock();
        for (auto& e : items) inner_->consume(e);
        mu_.lock();
    }

    std::shared_ptr<Sink> inner_;
    size_t batch_size_;
    int flush_ms_;
    std::vector<Event> buf_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::thread th_;
};

} // namespace crossbring

