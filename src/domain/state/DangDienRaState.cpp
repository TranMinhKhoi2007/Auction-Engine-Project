#include "auction/domain/state/DangDienRaState.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/state/HuyState.h"
#include "auction/domain/state/KetThucState.h"

namespace auction::domain {

void DangDienRaState::join(PhienDauGia& context, common::UserId userId) {
    context.addParticipant(userId);
}

void DangDienRaState::placeBid(PhienDauGia& context, const LuotTraGia& bid) {
    context.addBid(bid);
}

void DangDienRaState::close(PhienDauGia& context) {
    context.transitionTo(std::make_unique<KetThucState>());
}

void DangDienRaState::cancel(PhienDauGia& context, const std::string& /*reason*/) {
    context.setOutcome(AuctionOutcome::Cancelled);
    context.transitionTo(std::make_unique<HuyState>());
}

} // namespace auction::domain
