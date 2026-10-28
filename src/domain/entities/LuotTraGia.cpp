#include "auction/domain/entities/LuotTraGia.h"

namespace auction::domain {

LuotTraGia::LuotTraGia(common::BidId id, common::AuctionId auctionId, common::UserId bidderId,
                       Money amount, common::WallTime timestamp)
    : id_(id), auctionId_(auctionId), bidderId_(bidderId), amount_(amount), timestamp_(timestamp) {}

bool LuotTraGia::operator<(const LuotTraGia& other) const noexcept {
    if (amount_ != other.amount_) {
        return amount_ < other.amount_;
    }
    // Nếu bằng giá, lượt đặt sớm hơn được coi là lớn hơn
    // Do đó a < b khi timestamp của a muộn hơn b (a.timestamp_ > other.timestamp_)
    return timestamp_ > other.timestamp_;
}

bool LuotTraGia::operator>(const LuotTraGia& other) const noexcept {
    return other < *this;
}

bool LuotTraGia::operator==(const LuotTraGia& other) const noexcept {
    return id_ == other.id_ && auctionId_ == other.auctionId_ && bidderId_ == other.bidderId_ &&
           amount_ == other.amount_ && timestamp_ == other.timestamp_;
}

} // namespace auction::domain
