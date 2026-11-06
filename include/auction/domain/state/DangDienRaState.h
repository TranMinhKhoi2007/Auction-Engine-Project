#pragma once

#include "auction/domain/state/IAuctionState.h"

namespace auction::domain {

class DangDienRaState final : public IAuctionState {
public:
    [[nodiscard]] std::string name() const override {
        return "Đang diễn ra";
    }

    void join(PhienDauGia& context, common::UserId userId) override;
    void placeBid(PhienDauGia& context, const LuotTraGia& bid) override;
    void close(PhienDauGia& context) override;
    void cancel(PhienDauGia& context, const std::string& reason) override;
};

} // namespace auction::domain
