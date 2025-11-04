#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "crossbring/core/engine.h"

namespace crossbring {

class RecentBuffer; // fwd (stores last N events)

class HttpServer {
public:
    HttpServer(Engine& engine, std::shared_ptr<RecentBuffer> recent,
               const std::string& host = "127.0.0.1", int port = 9100);
    ~HttpServer();

    void start();
    void stop();

private:
    void run();

    Engine& engine_;
    std::shared_ptr<RecentBuffer> recent_;
    std::string host_;
    int port_;
    std::atomic<bool> running_{false};
    std::thread th_;
};

} // namespace crossbring

