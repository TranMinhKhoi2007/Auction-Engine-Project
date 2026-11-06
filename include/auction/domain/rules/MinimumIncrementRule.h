#pragma once

#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/rules/IBidRule.h"

namespace auction::domain {

class MinimumIncrementRule final : public IBidRule {
public:
    void validate(const PhienDauGia& auction,
                  const NguoiDung& /*bidder*/,
                  Money amount,
                  common::SteadyTime /*now*/) const override {
        if (auction.bidHistory().empty()) {
            if (amount < auction.startingPrice()) {
                throw InvalidBidException(BidRejectReason::BelowStartingPrice,
                                          "Giá đặt thấp hơn giá khởi điểm (" +
                                              auction.startingPrice().toString() + ")");
            }
        } else {
            Money minNext = auction.minimumNextBid();
            if (amount < minNext) {
                throw InvalidBidException(BidRejectReason::BelowMinimumIncrement,
                                          "Giá đặt phải tối thiểu là " + minNext.toString());
            }
        }
    }
};

} // namespace auction::domain
