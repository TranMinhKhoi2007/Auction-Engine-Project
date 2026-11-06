#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/entities/LuotTraGia.h"
#include "auction/domain/entities/SanPhamDauGia.h"
#include "auction/domain/events/AuctionEvent.h"
#include "auction/domain/events/AuctionEventPublisher.h"
#include "auction/domain/state/IAuctionState.h"
#include "auction/domain/values/Money.h"
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace auction::domain {

class PhienDauGia {
public:
    PhienDauGia(common::AuctionId id,
                std::unique_ptr<SanPhamDauGia> item,
                common::UserId sellerId,
                Money startingPrice,
                Money minIncrement,
                Money depositAmount,
                common::Millis startDelay,
                common::Millis duration);

    ~PhienDauGia();

    PhienDauGia(const PhienDauGia&) = delete;
    PhienDauGia& operator=(const PhienDauGia&) = delete;
    PhienDauGia(PhienDauGia&&) noexcept;
    PhienDauGia& operator=(PhienDauGia&&) noexcept;

    // Các hành động ủy quyền cho State
    void approve(common::SteadyTime now, common::WallTime wallNow);
    void start();
    void join(common::UserId userId);
    void acceptBid(const LuotTraGia& bid);
    void close();
    void cancel(const std::string& reason);

    // Thao tác nội bộ gọi từ State
    void transitionTo(std::unique_ptr<IAuctionState> newState);
    void addParticipant(common::UserId userId);
    void addBid(const LuotTraGia& bid);
    void setOutcome(AuctionOutcome outcome) noexcept {
        outcome_ = outcome;
    }

    void setClosedEarly(bool value) noexcept { closedEarly_ = value; }
    [[nodiscard]] bool closedEarly() const noexcept { return closedEarly_; }

    // Truy vấn
    [[nodiscard]] common::AuctionId id() const noexcept {
        return id_;
    }
    [[nodiscard]] const SanPhamDauGia& item() const noexcept {
        return *item_;
    }
    [[nodiscard]] common::UserId sellerId() const noexcept {
        return sellerId_;
    }
    [[nodiscard]] Money startingPrice() const noexcept {
        return startingPrice_;
    }
    [[nodiscard]] Money minIncrement() const noexcept {
        return minIncrement_;
    }
    [[nodiscard]] Money depositAmount() const noexcept {
        return depositAmount_;
    }
    [[nodiscard]] std::optional<common::SteadyTime> startTime() const noexcept {
        return startTime_;
    }
    [[nodiscard]] std::optional<common::SteadyTime> endTime() const noexcept {
        return endTime_;
    }
    [[nodiscard]] std::optional<common::WallTime> startWallTime() const noexcept {
        return startWallTime_;
    }
    [[nodiscard]] std::optional<common::WallTime> endWallTime() const noexcept {
        return endWallTime_;
    }
    [[nodiscard]] common::Millis startDelay() const noexcept { return startDelay_; }
    [[nodiscard]] common::Millis duration() const noexcept { return duration_; }
    [[nodiscard]] AuctionOutcome outcome() const noexcept {
        return outcome_;
    }
    [[nodiscard]] std::string stateName() const;

    [[nodiscard]] std::optional<LuotTraGia> highestBid() const noexcept;
    [[nodiscard]] Money currentPrice() const noexcept;
    [[nodiscard]] Money minimumNextBid() const noexcept;
    [[nodiscard]] std::span<const LuotTraGia> bidHistory() const noexcept {
        return bids_;
    }
    [[nodiscard]] std::span<const common::UserId> participants() const noexcept {
        return participants_;
    }
    [[nodiscard]] bool hasJoined(common::UserId userId) const noexcept;
    [[nodiscard]] common::Millis remainingTime(common::SteadyTime now) const noexcept;

    [[nodiscard]] AuctionEventPublisher& publisher() noexcept {
        return publisher_;
    }
    [[nodiscard]] const AuctionEventPublisher& publisher() const noexcept {
        return publisher_;
    }

private:
    friend class EntityCheckpoint;
    common::AuctionId id_;
    std::unique_ptr<SanPhamDauGia> item_;
    common::UserId sellerId_;
    Money startingPrice_;
    Money minIncrement_;
    Money depositAmount_;
    common::Millis startDelay_;
    common::Millis duration_;
    std::optional<common::SteadyTime> startTime_;
    std::optional<common::SteadyTime> endTime_;
    std::optional<common::WallTime> startWallTime_;
    std::optional<common::WallTime> endWallTime_;

    std::vector<LuotTraGia> bids_;
    std::vector<common::UserId> participants_;
    AuctionOutcome outcome_{AuctionOutcome::None};
    bool closedEarly_{};
    std::unique_ptr<IAuctionState> state_;
    AuctionEventPublisher publisher_;
};

} // namespace auction::domain
