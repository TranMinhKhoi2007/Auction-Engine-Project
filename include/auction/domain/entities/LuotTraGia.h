#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/values/Money.h"

namespace auction::domain {

class LuotTraGia {
public:
    LuotTraGia() = default;
    LuotTraGia(common::BidId id, common::AuctionId auctionId, common::UserId bidderId,
               Money amount, common::WallTime timestamp);

    [[nodiscard]] common::BidId id() const noexcept {
        return id_;
    }
    [[nodiscard]] common::AuctionId auctionId() const noexcept {
        return auctionId_;
    }
    [[nodiscard]] common::UserId bidderId() const noexcept {
        return bidderId_;
    }
    [[nodiscard]] Money amount() const noexcept {
        return amount_;
    }
    [[nodiscard]] common::WallTime timestamp() const noexcept {
        return timestamp_;
    }

    [[nodiscard]] bool operator<(const LuotTraGia& other) const noexcept;
    [[nodiscard]] bool operator>(const LuotTraGia& other) const noexcept;
    [[nodiscard]] bool operator==(const LuotTraGia& other) const noexcept;

private:
    common::BidId id_{};
    common::AuctionId auctionId_{};
    common::UserId bidderId_{};
    Money amount_{Money::zero()};
    common::WallTime timestamp_{};
};

} // namespace auction::domain
