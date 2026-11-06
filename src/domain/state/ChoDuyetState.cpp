#include "auction/domain/state/ChoDuyetState.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/state/HuyState.h"
#include "auction/domain/state/SapDienRaState.h"

namespace auction::domain {

void ChoDuyetState::approve(PhienDauGia& context) {
    context.transitionTo(std::make_unique<SapDienRaState>());
}

void ChoDuyetState::cancel(PhienDauGia& context, const std::string& /*reason*/) {
    context.setOutcome(AuctionOutcome::Cancelled);
    context.transitionTo(std::make_unique<HuyState>());
}

} // namespace auction::domain
