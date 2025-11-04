#pragma once

#include <memory>
#include <string>

#include "crossbring/event.h"

namespace crossbring {

class Sink {
public:
    virtual ~Sink() = default;
    virtual void consume(const Event& ev) = 0;
    virtual std::string name() const = 0;
};

} // namespace crossbring

