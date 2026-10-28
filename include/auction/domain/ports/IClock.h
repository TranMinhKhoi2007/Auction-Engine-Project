#pragma once

#include "auction/common/TimeTypes.h"
#include <chrono>

namespace auction::domain {

class IClock {
public:
    virtual ~IClock() = default;

    [[nodiscard]] virtual common::SteadyTime now() const = 0;
    [[nodiscard]] virtual common::WallTime wallNow() const = 0;
};

[[nodiscard]] inline common::SteadyTime toSteady(const IClock& clock, common::WallTime targetWall) {
    auto currentWall = clock.wallNow();
    auto diff = targetWall - currentWall;
    return clock.now() + std::chrono::duration_cast<common::SteadyTime::duration>(diff);
}

} // namespace auction::domain
