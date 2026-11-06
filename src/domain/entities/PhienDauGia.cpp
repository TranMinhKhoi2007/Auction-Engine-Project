#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/state/ChoDuyetState.h"
#include <algorithm>
#include <chrono>

namespace auction::domain {

PhienDauGia::PhienDauGia(common::AuctionId id,
                         std::unique_ptr<SanPhamDauGia> item,
                         common::UserId sellerId,
                         Money startingPrice,
                         Money minIncrement,
                         Money depositAmount,
                         common::Millis startDelay,
                         common::Millis duration)
    : id_(id),
      item_(std::move(item)),
      sellerId_(sellerId),
      startingPrice_(startingPrice),
      minIncrement_(minIncrement),
      depositAmount_(depositAmount),
      startDelay_(startDelay),
      duration_(duration),
      state_(std::make_unique<ChoDuyetState>()) {}

PhienDauGia::~PhienDauGia() = default;

PhienDauGia::PhienDauGia(PhienDauGia&&) noexcept = default;
PhienDauGia& PhienDauGia::operator=(PhienDauGia&&) noexcept = default;

void PhienDauGia::approve(common::SteadyTime now, common::WallTime wallNow) {
    const auto start = now + startDelay_;
    const auto end = start + duration_;
    const auto startWall = wallNow + startDelay_;
    const auto endWall = startWall + duration_;
    state_->approve(*this);
    // Optional time_point assignments cannot throw; illegal transitions leave the schedule intact.
    startTime_ = start;
    endTime_ = end;
    startWallTime_ = startWall;
    endWallTime_ = endWall;
}

void PhienDauGia::start() {
    state_->start(*this);
}

void PhienDauGia::join(common::UserId userId) {
    state_->join(*this, userId);
}

void PhienDauGia::acceptBid(const LuotTraGia& bid) {
    state_->placeBid(*this, bid);
}

void PhienDauGia::close() {
    state_->close(*this);
}

void PhienDauGia::cancel(const std::string& reason) {
    state_->cancel(*this, reason);
}

void PhienDauGia::transitionTo(std::unique_ptr<IAuctionState> newState) {
    const StateChangedEvent event{
        .auctionId = id_,
        .fromState = state_ ? state_->name() : "Chưa xác định",
        .toState = newState->name(),
        .timestamp = std::chrono::system_clock::now()
    };
    auto previous = std::move(state_);
    state_ = std::move(newState);
    try {
        publisher_.publish(event);
    } catch (...) {
        // Publisher allocates its snapshot before delivering any observer.
        state_ = std::move(previous);
        throw;
    }
}

void PhienDauGia::addParticipant(common::UserId userId) {
    if (hasJoined(userId)) {
        throw ValidationException("Người dùng đã tham gia phiên đấu giá này");
    }
    participants_.push_back(userId);
}

void PhienDauGia::addBid(const LuotTraGia& bid) {
    bids_.push_back(bid);
}

std::string PhienDauGia::stateName() const {
    return state_ ? state_->name() : "Không xác định";
}

std::optional<LuotTraGia> PhienDauGia::highestBid() const noexcept {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return *std::max_element(bids_.begin(), bids_.end());
}

Money PhienDauGia::currentPrice() const noexcept {
    auto highest = highestBid();
    if (highest.has_value()) {
        return highest->amount();
    }
    return startingPrice_;
}

Money PhienDauGia::minimumNextBid() const noexcept {
    auto highest = highestBid();
    if (highest.has_value()) {
        return highest->amount() + minIncrement_;
    }
    return startingPrice_;
}

bool PhienDauGia::hasJoined(common::UserId userId) const noexcept {
    return std::find(participants_.begin(), participants_.end(), userId) != participants_.end();
}

common::Millis PhienDauGia::remainingTime(common::SteadyTime now) const noexcept {
    if (!endTime_ || now >= *endTime_) {
        return common::Millis(0);
    }
    return std::chrono::duration_cast<common::Millis>(*endTime_ - now);
}

} // namespace auction::domain
