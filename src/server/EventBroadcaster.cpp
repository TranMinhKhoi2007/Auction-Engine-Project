#include "auction/server/EventBroadcaster.h"
#include "auction/domain/ports/IClock.h"
#include "auction/protocol/Envelope.h"
#include "auction/protocol/JsonCodec.h"
namespace auction::server {
void EventBroadcaster::onAuctionEvent(const domain::AuctionEvent &event) {
    std::optional<common::UserId> target;
    if(const auto *e=std::get_if<domain::OutbidEvent>(&event)) target=e->previousLeader;
    const auto now=std::chrono::duration_cast<common::Millis>(clock_.wallNow().time_since_epoch()).count();
    send_(protocol::encode({{"v",protocol::kVersion},{"event","event.auction"},
        {"seq",static_cast<double>(++sequence_)},{"serverNowMs",now},{"payload",protocol::toJson(event)}}),target);
}
}
