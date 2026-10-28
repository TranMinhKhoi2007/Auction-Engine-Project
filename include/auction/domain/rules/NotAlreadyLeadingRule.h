#pragma once

#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/rules/IBidRule.h"

namespace auction::domain {

class NotAlreadyLeadingRule final : public IBidRule {
public:
    void validate(const PhienDauGia& auction,
                  const NguoiDung& bidder,
                  Money /*amount*/,
                  common::SteadyTime /*now*/) const override {
        auto highest = auction.highestBid();
        if (highest.has_value() && highest->bidderId() == bidder.id()) {
            throw InvalidBidException(BidRejectReason::AlreadyLeading,
                                      "Bạn hiện đang là người trả giá cao nhất");
        }
    }
};

} // namespace auction::domain
