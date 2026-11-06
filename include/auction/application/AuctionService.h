#pragma once

#include "auction/application/AuctionScheduler.h"
#include "auction/application/Commands.h"
#include "auction/application/ICommissionPolicy.h"
#include "auction/domain/ports/IAuctionRepository.h"
#include "auction/domain/ports/IClock.h"
#include "auction/domain/ports/IUserRepository.h"
#include <memory>
#include <vector>

namespace auction::application {

class AuctionService {
public:
    AuctionService(domain::IAuctionRepository& auctionRepo,
                   domain::IUserRepository& userRepo,
                   AuctionScheduler& scheduler,
                   std::shared_ptr<ICommissionPolicy> commissionPolicy,
                   const domain::IClock& clock);

    common::AuctionId createAuction(const CreateAuctionCommand& command);
    void approveAuction(common::AuctionId id, common::UserId actingUserId);
    void startAuction(common::AuctionId id);
    void closeAuctionEarly(common::AuctionId id, common::UserId actingUserId);
    void joinAuction(common::AuctionId id, common::UserId userId);
    void closeAuction(common::AuctionId id);
    void cancelAuction(common::AuctionId id, common::UserId actingUserId, const std::string& reason);
    void deleteAuction(common::AuctionId id, common::UserId actingUserId);

    void followAuction(common::AuctionId id, common::UserId userId);
    void unfollowAuction(common::AuctionId id, common::UserId userId);

    void addGlobalObserver(std::weak_ptr<domain::IAuctionObserver> observer);

private:
    void finalizeAuction(domain::PhienDauGia& auction, bool closedEarly);

    domain::IAuctionRepository& auctionRepo_;
    domain::IUserRepository& userRepo_;
    AuctionScheduler& scheduler_;
    std::shared_ptr<ICommissionPolicy> commissionPolicy_;
    const domain::IClock& clock_;
    std::vector<std::weak_ptr<domain::IAuctionObserver>> globalObservers_;

    uint64_t nextAuctionId_{1};
    uint64_t nextProductId_{1};
};

} // namespace auction::application
