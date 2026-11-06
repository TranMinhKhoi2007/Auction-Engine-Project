#pragma once

#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/rules/IBidRule.h"

namespace auction::domain {

class AuctionActiveRule final : public IBidRule {
public:
    void validate(const PhienDauGia& auction,
                  const NguoiDung& /*bidder*/,
                  Money /*amount*/,
                  common::SteadyTime now) const override {
        if (auction.stateName() != "Đang diễn ra") {
            throw InvalidBidException(BidRejectReason::AuctionNotActive,
                                      "Phiên đấu giá hiện không ở trạng thái Đang diễn ra");
        }
        if (auction.endTime() && now >= *auction.endTime()) {
            throw InvalidBidException(BidRejectReason::AuctionExpired,
                                      "Phiên đấu giá đã hết thời gian đặt giá");
        }
    }
};

} // namespace auction::domain
