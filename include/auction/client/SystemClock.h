#pragma once
#include "auction/domain/ports/IClock.h"
namespace auction::client {
class SystemClock final : public domain::IClock {
public:
    common::SteadyTime now() const override {return std::chrono::steady_clock::now();}
    common::WallTime wallNow() const override {return std::chrono::system_clock::now();}
};
}
