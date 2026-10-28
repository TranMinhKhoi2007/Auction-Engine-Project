#include "auction/domain/state/SapDienRaState.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/state/DangDienRaState.h"
#include "auction/domain/state/HuyState.h"

namespace auction::domain {

void SapDienRaState::start(PhienDauGia& context) {
    context.transitionTo(std::make_unique<DangDienRaState>());
}

void SapDienRaState::join(PhienDauGia& context, common::UserId userId) {
    context.addParticipant(userId);
}

void SapDienRaState::cancel(PhienDauGia& context, const std::string& /*reason*/) {
    context.setOutcome(AuctionOutcome::Cancelled);
    context.transitionTo(std::make_unique<HuyState>());
}

} // namespace auction::domain
