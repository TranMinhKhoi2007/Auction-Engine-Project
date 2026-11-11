#pragma once

#include "auction/domain/events/IAuctionObserver.h"
#include "auction/domain/exceptions/AuctionException.h"
#include "auction/domain/ports/IUserRepository.h"
#include <fstream>
#include <mutex>
#include <string>

namespace auction::infrastructure {

class BidHistoryLogObserver final : public domain::IAuctionObserver {
public:
    explicit BidHistoryLogObserver(const std::string& logFilePath,
                                   const domain::IUserRepository* userRepo = nullptr);
    ~BidHistoryLogObserver() override;

    void onAuctionEvent(const domain::AuctionEvent& event) override;

    void logRejectedBid(common::AuctionId auctionId,
                        common::UserId userId,
                        domain::Money amount,
                        domain::BidRejectReason reason);

    void logRefund(common::AuctionId auctionId, common::UserId userId, domain::Money amount);
    void logForfeit(common::AuctionId auctionId, common::UserId userId, domain::Money amount);

private:
    void writeLine(const std::string& line);
    std::string getUserDisplay(common::UserId id);

    std::ofstream fileStream_;
    std::mutex mutex_;
    const domain::IUserRepository* userRepo_{nullptr};
};

} // namespace auction::infrastructure
