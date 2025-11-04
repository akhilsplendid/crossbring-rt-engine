#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include "crossbring/event.h"
#include "queue.h"

namespace crossbring {

class Sink;

class Engine {
public:
    using Processor = std::function<void(Event&)>; // in-place mutation allowed

    explicit Engine(size_t queue_capacity = 1024, size_t workers = std::thread::hardware_concurrency(), bool drop_on_full = false);
    ~Engine();

    void start();
    void stop();

    bool submit(Event ev);

    void add_processor(Processor p);
    void add_sink(std::shared_ptr<Sink> sink);

    // Metrics
    uint64_t processed_count() const { return processed_.load(std::memory_order_relaxed); }
    uint64_t dropped_count() const { return dropped_.load(std::memory_order_relaxed); }
    size_t queue_size() const { return queue_.size(); }

private:
    void worker_loop();

    BoundedQueue<Event> queue_;
    std::vector<std::thread> workers_;
    std::vector<Processor> processors_;
    std::vector<std::shared_ptr<Sink>> sinks_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> processed_{0};
    std::atomic<uint64_t> dropped_{0};
    bool drop_on_full_{false};
};

} // namespace crossbring
