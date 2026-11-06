#pragma once

#include "auction/domain/state/IAuctionState.h"

namespace auction::domain {

class HuyState final : public IAuctionState {
public:
    [[nodiscard]] std::string name() const override {
        return "Hủy";
    }
    // Trạng thái cuối: mọi hành động đều ném InvalidStateTransitionException kế thừa từ IAuctionState
};

} // namespace auction::domain
