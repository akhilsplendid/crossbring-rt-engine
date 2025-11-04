#include "crossbring/core/engine.h"

#include <spdlog/spdlog.h>
#include <mutex>

#include "crossbring/sinks/sink.h"

namespace crossbring {

Engine::Engine(size_t queue_capacity, size_t workers)
    : queue_(queue_capacity) {
    if (workers == 0) workers = 1;
    workers_.reserve(workers);
}

Engine::~Engine() { stop(); }

void Engine::start() {
    if (running_.exchange(true)) return;
    spdlog::info("Engine starting with {} worker(s)", workers_.capacity());
    for (size_t i = 0; i < workers_.capacity(); ++i) {
        workers_.emplace_back([this]{ worker_loop(); });
    }
}

void Engine::stop() {
    if (!running_.exchange(false)) return;
    queue_.stop();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
    spdlog::info("Engine stopped. processed={}, dropped={}", processed_.load(), dropped_.load());
}

bool Engine::submit(Event ev) {
    if (!running_.load(std::memory_order_relaxed)) return false;
    if (!queue_.push(std::move(ev))) {
        dropped_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return true;
}

void Engine::add_processor(Processor p) { processors_.push_back(std::move(p)); }

void Engine::add_sink(std::shared_ptr<Sink> sink) { sinks_.push_back(std::move(sink)); }

void Engine::worker_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        auto item = queue_.pop();
        if (!item.has_value()) break;
        auto& ev = item.value();
        for (auto& p : processors_) {
            p(ev);
        }
        for (auto& s : sinks_) {
            s->consume(ev);
        }
        processed_.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace crossbring

