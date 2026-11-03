#pragma once

#include "auction/common/StrongId.h"
#include "auction/domain/ports/IAuctionRepository.h"
#include "auction/domain/ports/IClock.h"
#include "auction/domain/ports/IUserRepository.h"
#include "auction/domain/rules/BidValidator.h"
#include "auction/domain/values/Money.h"

namespace auction::application {

class BiddingService {
public:
    BiddingService(domain::IAuctionRepository& auctionRepo,
                   domain::IUserRepository& userRepo,
                   const domain::IClock& clock,
                   domain::BidValidator validator);

    void placeBid(common::AuctionId auctionId, common::UserId bidderId, domain::Money amount);

private:
    domain::IAuctionRepository& auctionRepo_;
    domain::IUserRepository& userRepo_;
    const domain::IClock& clock_;
    domain::BidValidator validator_;
    uint64_t nextBidId_{1};
};

} // namespace auction::application
