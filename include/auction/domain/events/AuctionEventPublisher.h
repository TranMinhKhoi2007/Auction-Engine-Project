#pragma once

#include "auction/domain/events/AuctionEvent.h"
#include "auction/domain/events/IAuctionObserver.h"
#include <memory>
#include <vector>

namespace auction::domain {

class AuctionEventPublisher {
public:
    AuctionEventPublisher() = default;

    void subscribe(std::weak_ptr<IAuctionObserver> observer);
    void unsubscribe(const std::shared_ptr<IAuctionObserver>& observer);
    void publish(const AuctionEvent& event);

    [[nodiscard]] size_t observerCount() const;

private:
    std::vector<std::weak_ptr<IAuctionObserver>> observers_;
};

} // namespace auction::domain
