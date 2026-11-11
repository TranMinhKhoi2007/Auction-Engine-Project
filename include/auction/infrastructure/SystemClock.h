#pragma once

#include "auction/domain/ports/IClock.h"

namespace auction::infrastructure {

class SystemClock final : public domain::IClock {
public:
    [[nodiscard]] common::SteadyTime now() const override {
        return std::chrono::steady_clock::now();
    }

    [[nodiscard]] common::WallTime wallNow() const override {
        return std::chrono::system_clock::now();
    }
};

} // namespace auction::infrastructure
