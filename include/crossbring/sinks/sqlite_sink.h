#pragma once

#include <memory>
#include <string>
#include "sink.h"

namespace crossbring {

#ifdef USE_SQLITE
std::shared_ptr<Sink> make_sqlite_sink(const std::string& path);
#endif

} // namespace crossbring

