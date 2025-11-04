#ifdef USE_ZEROMQ

#include <zmq.h>
#include <string>
#include <spdlog/spdlog.h>

#include "crossbring/sinks/sink.h"

namespace crossbring {

class ZmqPubSink : public Sink {
public:
    ZmqPubSink(const std::string& endpoint) {
        ctx_ = zmq_ctx_new();
        sock_ = zmq_socket(ctx_, ZMQ_PUB);
        if (zmq_bind(sock_, endpoint.c_str()) != 0) {
            throw std::runtime_error(std::string("ZMQ bind failed: ") + zmq_strerror(zmq_errno()));
        }
        endpoint_ = endpoint;
    }
    ~ZmqPubSink() override {
        if (sock_) zmq_close(sock_);
        if (ctx_) zmq_ctx_term(ctx_);
    }
    void consume(const Event& ev) override {
        std::string data = ev.payload.dump();
        int rc = zmq_send(sock_, data.data(), data.size(), ZMQ_DONTWAIT);
        if (rc < 0) {
            spdlog::debug("ZMQ send failed: {}", zmq_strerror(zmq_errno()));
        }
    }
    std::string name() const override { return std::string("zmq_pub(") + endpoint_ + ")"; }
private:
    void* ctx_ = nullptr;
    void* sock_ = nullptr;
    std::string endpoint_;
};

std::shared_ptr<Sink> make_zmq_pub_sink(const std::string& endpoint) {
    return std::make_shared<ZmqPubSink>(endpoint);
}

} // namespace crossbring

#endif // USE_ZEROMQ
