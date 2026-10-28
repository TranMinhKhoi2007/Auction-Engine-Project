#pragma once

#include "auction/common/StrongId.h"
#include <string>

namespace auction::domain {

class PhienDauGia;
class LuotTraGia;

class IAuctionState {
public:
    virtual ~IAuctionState() = default;

    [[nodiscard]] virtual std::string name() const = 0;

    virtual void approve(PhienDauGia& context);
    virtual void start(PhienDauGia& context);
    virtual void join(PhienDauGia& context, common::UserId userId);
    virtual void placeBid(PhienDauGia& context, const LuotTraGia& bid);
    virtual void close(PhienDauGia& context);
    virtual void cancel(PhienDauGia& context, const std::string& reason);
};

} // namespace auction::domain
