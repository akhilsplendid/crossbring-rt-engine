#pragma once

#include <memory>
#include "sink.h"

namespace crossbring {

std::shared_ptr<Sink> make_console_sink();

} // namespace crossbring

