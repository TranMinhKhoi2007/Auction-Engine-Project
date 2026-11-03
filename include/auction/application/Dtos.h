#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/entities/LuotTraGia.h"
#include "auction/domain/events/AuctionEvent.h"
#include "auction/domain/values/Money.h"
#include <optional>
#include <string>
#include <vector>

namespace auction::application {

// Private account projection, returned only to its authenticated owner.
struct AccountDto {
    common::UserId id;
    std::string name;
    std::string email;
};

struct UserDto {
    common::UserId id;
    std::string name;
    domain::Money availableBalance;
    domain::Money heldBalance;
    domain::Money totalBalance;
};

struct AuctionSummaryDto {
    common::AuctionId id;
    std::string productName;
    std::string category;
    std::string state;
    domain::Money currentPrice;
    common::Millis remainingTime;
};

struct AuctionDetailDto {
    common::AuctionId id;
    common::ProductId productId;
    std::string productName;
    std::string category;
    std::string productDescription;
    common::UserId sellerId;
    std::string sellerName;
    domain::Money startingPrice;
    domain::Money minIncrement;
    domain::Money depositAmount;
    domain::Money currentPrice;
    domain::Money minimumNextBid;
    std::optional<common::UserId> leaderId;
    std::string leaderName;
    std::string state;
    domain::AuctionOutcome outcome;
    bool closedEarly{};
    std::optional<common::WallTime> startWallTime;
    std::optional<common::WallTime> endWallTime;
    common::Millis startDelayMs;
    common::Millis durationMs;
    common::Millis remainingTime;
    std::vector<common::UserId> participants;
    std::vector<domain::LuotTraGia> bidHistory;
    bool hasImage{};
};

struct WalletHoldEntry {
    common::AuctionId auctionId;
    std::string auctionTitle;
    domain::Money amount;
};

struct WalletDto {
    domain::Money available;
    domain::Money totalHeld;
    domain::Money total;
    std::vector<WalletHoldEntry> holds;
};

} // namespace auction::application
