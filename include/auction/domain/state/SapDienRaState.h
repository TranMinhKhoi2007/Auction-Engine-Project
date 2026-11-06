#pragma once

#include "auction/domain/state/IAuctionState.h"

namespace auction::domain {

class SapDienRaState final : public IAuctionState {
public:
    [[nodiscard]] std::string name() const override {
        return "Sắp diễn ra";
    }

    void start(PhienDauGia& context) override;
    void join(PhienDauGia& context, common::UserId userId) override;
    void cancel(PhienDauGia& context, const std::string& reason) override;
};

} // namespace auction::domain
