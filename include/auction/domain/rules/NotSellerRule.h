#pragma once

#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/rules/IBidRule.h"

namespace auction::domain {

class NotSellerRule final : public IBidRule {
public:
    void validate(const PhienDauGia& auction,
                  const NguoiDung& bidder,
                  Money /*amount*/,
                  common::SteadyTime /*now*/) const override {
        if (bidder.id() == auction.sellerId()) {
            throw InvalidBidException(BidRejectReason::SellerCannotBid,
                                      "Người bán không được phép đặt giá cho sản phẩm của chính mình");
        }
    }
};

} // namespace auction::domain
