#pragma once

#include "auction/domain/events/AuctionEvent.h"

namespace auction::domain {

class IAuctionObserver {
public:
    virtual ~IAuctionObserver() = default;
    virtual void onAuctionEvent(const AuctionEvent& event) = 0;
};

} // namespace auction::domain
