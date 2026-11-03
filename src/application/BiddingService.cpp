#include "auction/application/BiddingService.h"
#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"
#include <algorithm>

namespace auction::application {

BiddingService::BiddingService(domain::IAuctionRepository& auctionRepo,
                               domain::IUserRepository& userRepo,
                               const domain::IClock& clock,
                               domain::BidValidator validator)
    : auctionRepo_(auctionRepo),
      userRepo_(userRepo),
      clock_(clock),
      validator_(std::move(validator)) {
    for(const auto &a:auctionRepo_.findAll())for(const auto &bid:a->bidHistory())nextBidId_=std::max(nextBidId_,bid.id().value()+1);
}

void BiddingService::placeBid(common::AuctionId auctionId, common::UserId bidderId, domain::Money amount) {
    auto auction = auctionRepo_.findById(auctionId);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", auctionId.value());
    }

    auto user = userRepo_.findById(bidderId);
    if (!user) {
        throw domain::EntityNotFoundException("Người dùng", bidderId.value());
    }

    auto participant = user;

    // Xác thực các quy tắc nghiệp vụ
    validator_.validate(*auction, *participant, amount, clock_.now());

    // Ghi nhận người dẫn đầu trước khi đặt giá mới để phát hiện OutbidEvent
    auto prevLeader = auction->highestBid().has_value()
                          ? std::make_optional(auction->highestBid()->bidderId())
                          : std::nullopt;

    // Tạo lượt đặt giá mới (không di chuyển tiền trong ví)
    domain::LuotTraGia bid(common::BidId{nextBidId_++}, auctionId, bidderId, amount, clock_.wallNow());

    // Đưa vào phiên đấu giá
    auction->acceptBid(bid);

    // Phát sự kiện đặt giá thành công
    auction->publisher().publish(domain::BidPlacedEvent{
        .auctionId = auctionId,
        .bidderId = bidderId,
        .amount = amount,
        .timestamp = clock_.wallNow()
    });

    // Nếu có người dẫn đầu trước đó và bị vượt giá
    if (prevLeader.has_value() && *prevLeader != bidderId) {
        auction->publisher().publish(domain::OutbidEvent{
            .auctionId = auctionId,
            .previousLeader = *prevLeader,
            .newAmount = amount,
            .timestamp = clock_.wallNow()
        });
    }
}

} // namespace auction::application
