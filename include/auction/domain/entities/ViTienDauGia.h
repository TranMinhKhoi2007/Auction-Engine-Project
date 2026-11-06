#pragma once

#include "auction/common/StrongId.h"
#include "auction/domain/values/Money.h"
#include <unordered_map>

namespace auction::domain {

class ViTienDauGia {
public:
    ViTienDauGia() = default;
    explicit ViTienDauGia(Money initialBalance);

    void topUp(Money amount);
    void hold(common::AuctionId auctionId, Money amount);
    Money release(common::AuctionId auctionId) noexcept;

    [[nodiscard]] bool canSettle(common::AuctionId auctionId, Money price) const noexcept;
    void settle(common::AuctionId auctionId, Money price);

    [[nodiscard]] Money forfeit(common::AuctionId auctionId);
    void receive(Money amount);

    [[nodiscard]] Money available() const noexcept {
        return available_;
    }
    [[nodiscard]] Money heldFor(common::AuctionId auctionId) const noexcept;
    [[nodiscard]] Money totalHeld() const noexcept;
    [[nodiscard]] Money total() const noexcept;
    [[nodiscard]] const std::unordered_map<common::AuctionId, Money>& holds() const noexcept {
        return holds_;
    }

private:
    Money available_{Money::zero()};
    std::unordered_map<common::AuctionId, Money> holds_;
};

} // namespace auction::domain
