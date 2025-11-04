#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>

namespace crossbring {

// Simple bounded MPMC queue with condition variables
template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t capacity) : capacity_(capacity) {}

    bool push(T item) {
        std::unique_lock<std::mutex> lock(m_);
        not_full_cv_.wait(lock, [&]{ return stop_ || q_.size() < capacity_; });
        if (stop_) return false;
        q_.push(std::move(item));
        not_empty_cv_.notify_one();
        return true;
    }

    bool try_push(T item) {
        std::lock_guard<std::mutex> lock(m_);
        if (stop_ || q_.size() >= capacity_) return false;
        q_.push(std::move(item));
        not_empty_cv_.notify_one();
        return true;
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_cv_.wait(lock, [&]{ return stop_ || !q_.empty(); });
        if (stop_ && q_.empty()) return std::nullopt;
        T item = std::move(q_.front());
        q_.pop();
        not_full_cv_.notify_one();
        return item;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_);
            stop_ = true;
        }
        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.size();
    }

private:
    size_t capacity_;
    std::queue<T> q_;
    mutable std::mutex m_;
    std::condition_variable not_empty_cv_;
    std::condition_variable not_full_cv_;
    bool stop_ = false;
};

} // namespace crossbring
