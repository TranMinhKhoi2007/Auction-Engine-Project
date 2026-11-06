#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/values/Money.h"
#include <optional>
#include <string>
#include <variant>

namespace auction::domain {

enum class AuctionOutcome {
    None,
    Sold,
    NoBids,
    PaymentDefault,
    Cancelled
};

[[nodiscard]] std::string toVietnameseString(AuctionOutcome outcome);

struct BidPlacedEvent {
    common::AuctionId auctionId;
    common::UserId bidderId;
    Money amount;
    common::WallTime timestamp;
};

struct OutbidEvent {
    common::AuctionId auctionId;
    common::UserId previousLeader;
    Money newAmount;
    common::WallTime timestamp;
};

struct ParticipantJoinedEvent {
    common::AuctionId auctionId;
    common::UserId userId;
    Money deposit;
    common::WallTime timestamp;
};

struct StateChangedEvent {
    common::AuctionId auctionId;
    std::string fromState;
    std::string toState;
    common::WallTime timestamp;
};

struct CountdownEvent {
    common::AuctionId auctionId;
    int64_t secondsRemaining;
    common::WallTime timestamp;
};

struct AuctionClosedEvent {
    common::AuctionId auctionId;
    AuctionOutcome outcome;
    std::optional<common::UserId> winnerId;
    Money finalPrice;
    common::WallTime timestamp;
    bool closedEarly{false};
};

struct AuctionCancelledEvent {
    common::AuctionId auctionId;
    std::string reason;
    common::WallTime timestamp;
};

using AuctionEvent = std::variant<
    BidPlacedEvent,
    OutbidEvent,
    ParticipantJoinedEvent,
    StateChangedEvent,
    CountdownEvent,
    AuctionClosedEvent,
    AuctionCancelledEvent
>;

} // namespace auction::domain
