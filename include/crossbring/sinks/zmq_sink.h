#pragma once

#include <memory>
#include <string>
#include "sink.h"

namespace crossbring {

#ifdef USE_ZEROMQ
std::shared_ptr<Sink> make_zmq_pub_sink(const std::string& endpoint);
#endif

} // namespace crossbring

