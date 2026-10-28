#pragma once

#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/rules/IBidRule.h"

namespace auction::domain {

class SufficientFundsRule final : public IBidRule {
public:
    void validate(const PhienDauGia& auction,
                  const NguoiDung& bidder,
                  Money amount,
                  common::SteadyTime /*now*/) const override {
        // Kiểm tra khả năng thanh toán: số dư khả dụng + tiền cọc đang giữ cho phiên này
        int64_t availablePlusHeld = bidder.wallet().available().amount() +
                                    bidder.wallet().heldFor(auction.id()).amount();
        if (availablePlusHeld < amount.amount()) {
            throw InvalidBidException(BidRejectReason::InsufficientFunds,
                                      "Tài khoản không đủ khả năng thanh toán nếu trúng thầu mức giá này");
        }
    }
};

} // namespace auction::domain
