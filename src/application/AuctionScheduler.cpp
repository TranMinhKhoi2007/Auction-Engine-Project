#include "auction/application/AuctionScheduler.h"
#include "auction/domain/AuctionConfig.h"

namespace auction::application {

AuctionScheduler::AuctionScheduler(infrastructure::EventLoop& eventLoop,
                                   const domain::IClock& clock,
                                   AuctionCallback onStart,
                                   AuctionCallback onClose,std::shared_ptr<domain::IUnitOfWork> unitOfWork)
    : eventLoop_(eventLoop),
      clock_(clock),
      onStart_(std::move(onStart)),
      onClose_(std::move(onClose)),unitOfWork_(std::move(unitOfWork)) {}

void AuctionScheduler::scheduleAuction(const std::shared_ptr<domain::PhienDauGia>& auction) {
    if (!auction || !auction->startTime() || !auction->endTime()) {
        return;
    }

    common::AuctionId id = auction->id();
    cancelAuctionEvents(id);

    std::vector<common::EventId> eventIds;
    // 1. Lên lịch Bắt đầu (start), lịch đã tính từ lúc duyệt.
    if(auction->stateName()=="Sắp diễn ra") {
    auto startId = eventLoop_.schedule(*auction->startTime(), [this, id]() {
        if (onStart_) {
            onStart_(id);
        }
    });
    eventIds.push_back(startId);
    }

    // 2. Lên lịch Kết thúc (close)
    auto closeId = eventLoop_.schedule(*auction->endTime(), [this, id]() {
        if (onClose_) {
            onClose_(id);
        }
    });
    eventIds.push_back(closeId);

    // 3. Lên lịch Countdown định kỳ mỗi 1s
    std::weak_ptr<domain::PhienDauGia> weakAuction = auction;
    auto countdownId = eventLoop_.scheduleEvery(common::Millis(1000), [this, weakAuction, id]() {
        auto a = weakAuction.lock();
        if (!a) {
            return;
        }
        if (a->stateName() != "Đang diễn ra") {
            return;
        }
        auto remMillis = a->remainingTime(clock_.now());
        int64_t remSec = remMillis.count() / 1000;
        if (remSec <= 0) {
            return;
        }

        int64_t coarseSec = std::chrono::duration_cast<common::Seconds>(domain::kCountdownCoarseInterval).count();
        if (remSec <= domain::kCountdownLastSeconds || (coarseSec > 0 && remSec % coarseSec == 0)) {
            const auto publish=[&]{a->publisher().publish(domain::CountdownEvent{
                .auctionId = id,
                .secondsRemaining = remSec,
                .timestamp = clock_.wallNow()
            });};
            if(unitOfWork_)unitOfWork_->execute({"COUNTDOWN",{},id},publish);else publish();
        }
    });
    eventIds.push_back(countdownId);

    scheduledEvents_[id] = std::move(eventIds);
}

void AuctionScheduler::cancelAuctionEvents(common::AuctionId id) {
    auto it = scheduledEvents_.find(id);
    if (it != scheduledEvents_.end()) {
        for (auto eventId : it->second) {
            eventLoop_.cancel(eventId);
        }
        scheduledEvents_.erase(it);
    }
}

} // namespace auction::application
