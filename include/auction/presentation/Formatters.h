#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/events/AuctionEvent.h"
#include "auction/domain/values/Money.h"

#include <QString>

namespace auction::presentation {

class Formatters {
public:
    static QString formatMoney(domain::Money money);
    static QString formatTimeRemaining(common::Millis remaining);
    static QString formatWallTime(common::WallTime wallTime);
    static QString formatAuctionOutcome(domain::AuctionOutcome outcome);
    static QString formatBidRejectReason(const QString &reason);
    static QString formatEventSummary(const domain::AuctionEvent &event,
                                      common::UserId currentUserId);
};

} // namespace auction::presentation
