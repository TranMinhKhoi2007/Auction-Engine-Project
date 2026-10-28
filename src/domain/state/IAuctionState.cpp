#include "auction/domain/state/IAuctionState.h"
#include "auction/domain/exceptions/AuctionException.h"

namespace auction::domain {

void IAuctionState::approve(PhienDauGia& /*context*/) {
    throw InvalidStateTransitionException(name(), "Duyệt phiên (approve)");
}

void IAuctionState::start(PhienDauGia& /*context*/) {
    throw InvalidStateTransitionException(name(), "Bắt đầu phiên (start)");
}

void IAuctionState::join(PhienDauGia& /*context*/, common::UserId /*userId*/) {
    throw InvalidStateTransitionException(name(), "Tham gia phiên (join)");
}

void IAuctionState::placeBid(PhienDauGia& /*context*/, const LuotTraGia& /*bid*/) {
    throw InvalidStateTransitionException(name(), "Đặt giá (placeBid)");
}

void IAuctionState::close(PhienDauGia& /*context*/) {
    throw InvalidStateTransitionException(name(), "Đóng phiên (close)");
}

void IAuctionState::cancel(PhienDauGia& /*context*/, const std::string& /*reason*/) {
    throw InvalidStateTransitionException(name(), "Hủy phiên (cancel)");
}

} // namespace auction::domain
