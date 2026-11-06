#pragma once

#include "auction/common/StrongId.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/ports/IClock.h"
#include "auction/domain/ports/IUnitOfWork.h"
#include "auction/infrastructure/EventLoop.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace auction::application {

class AuctionScheduler {
public:
    using AuctionCallback = std::function<void(common::AuctionId)>;

    AuctionScheduler(infrastructure::EventLoop& eventLoop,
                     const domain::IClock& clock,
                     AuctionCallback onStart,
                     AuctionCallback onClose,
                     std::shared_ptr<domain::IUnitOfWork> unitOfWork={});

    void scheduleAuction(const std::shared_ptr<domain::PhienDauGia>& auction);
    void cancelAuctionEvents(common::AuctionId id);

private:
    infrastructure::EventLoop& eventLoop_;
    const domain::IClock& clock_;
    AuctionCallback onStart_;
    AuctionCallback onClose_;
    std::shared_ptr<domain::IUnitOfWork> unitOfWork_;
    std::unordered_map<common::AuctionId, std::vector<common::EventId>> scheduledEvents_;
};

} // namespace auction::application
