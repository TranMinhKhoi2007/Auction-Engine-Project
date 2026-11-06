#pragma once

#include "auction/domain/state/IAuctionState.h"

namespace auction::domain {

class ChoDuyetState final : public IAuctionState {
public:
    [[nodiscard]] std::string name() const override {
        return "Chờ duyệt";
    }

    void approve(PhienDauGia& context) override;
    void cancel(PhienDauGia& context, const std::string& reason) override;
};

} // namespace auction::domain
