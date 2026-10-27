#pragma once
#include "auction/domain/events/IAuctionObserver.h"
#include "auction/domain/ports/IClock.h"
#include <functional>
#include <QString>
namespace auction::server {
class EventBroadcaster final : public domain::IAuctionObserver {
public:
    using Send=std::function<void(const QString &,std::optional<common::UserId>)>;
    EventBroadcaster(const domain::IClock &clock,Send send):clock_(clock),send_(std::move(send)) {}
    void onAuctionEvent(const domain::AuctionEvent &event) override;
private:
    const domain::IClock &clock_;
    Send send_;
    quint64 sequence_{};
};
}
