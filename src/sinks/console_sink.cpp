#include <spdlog/spdlog.h>

#include "crossbring/sinks/sink.h"

namespace crossbring {

class ConsoleSink : public Sink {
public:
    void consume(const Event& ev) override {
        try {
            auto title = ev.payload.contains("title") ? ev.payload["title"].get<std::string>() : std::string{};
            auto value = ev.payload.contains("value") ? ev.payload["value"].dump() : std::string{};
            if (!title.empty()) {
                spdlog::info("[{}] key={} title={}", ev.source, ev.key, title);
            } else if (!value.empty()) {
                spdlog::debug("[{}] key={} value={}", ev.source, ev.key, value);
            } else {
                spdlog::debug("[{}] key={} payload-size={}B", ev.source, ev.key, ev.payload.dump().size());
            }
        } catch (...) {}
    }
    std::string name() const override { return "console"; }
};

// Provide a factory function so apps can create it without exposing class
std::shared_ptr<Sink> make_console_sink() { return std::make_shared<ConsoleSink>(); }

} // namespace crossbring

