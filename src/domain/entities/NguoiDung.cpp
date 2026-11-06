#include "auction/domain/entities/NguoiDung.h"
#include <algorithm>

namespace auction::domain {


void NguoiDung::removeAuction(common::AuctionId id) {
    auto it = std::remove(postedAuctions_.begin(), postedAuctions_.end(), id);
    postedAuctions_.erase(it, postedAuctions_.end());
}

void NguoiDung::removeAuctionEvents(common::AuctionId auctionId) {
    auto it = std::remove_if(inbox_.begin(), inbox_.end(), [auctionId](const AuctionEvent &event) {
        return std::visit([auctionId](const auto &e) { return e.auctionId == auctionId; }, event);
    });
    inbox_.erase(it, inbox_.end());
}

void NguoiDung::follow(common::AuctionId auctionId) {
    if (!isFollowing(auctionId)) {
        followedAuctions_.push_back(auctionId);
    }
}

void NguoiDung::unfollow(common::AuctionId auctionId) {
    auto it = std::remove(followedAuctions_.begin(), followedAuctions_.end(), auctionId);
    followedAuctions_.erase(it, followedAuctions_.end());
}

bool NguoiDung::isFollowing(common::AuctionId auctionId) const noexcept {
    return std::find(followedAuctions_.begin(), followedAuctions_.end(), auctionId) != followedAuctions_.end();
}

void NguoiDung::onAuctionEvent(const AuctionEvent& event) {
    bool shouldAdd = false;

    std::visit(
        [this, &shouldAdd](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, OutbidEvent>) {
                // OutbidEvent chỉ thêm vào inbox của người bị vượt giá
                if (e.previousLeader == id()) {
                    shouldAdd = true;
                }
            } else {
                // Các event khác: chỉ nhận nếu đang theo dõi phiên
                if (isFollowing(e.auctionId)) {
                    shouldAdd = true;
                }
            }
        },
        event);

    if (shouldAdd) {
        if (inbox_.size() >= kMaxInboxSize) {
            inbox_.pop_front();
        }
        inbox_.push_back(event);
    }
}

} // namespace auction::domain
