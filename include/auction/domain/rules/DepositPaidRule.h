#pragma once

#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/rules/IBidRule.h"

namespace auction::domain {

class DepositPaidRule final : public IBidRule {
public:
    void validate(const PhienDauGia& auction,
                  const NguoiDung& bidder,
                  Money /*amount*/,
                  common::SteadyTime /*now*/) const override {
        if (!auction.hasJoined(bidder.id())) {
            throw InvalidBidException(BidRejectReason::DepositNotPaid,
                                      "Bạn chưa đặt cọc ký quỹ để tham gia phiên đấu giá này");
        }
    }
};

} // namespace auction::domain
