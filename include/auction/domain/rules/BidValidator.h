#pragma once

#include "auction/domain/rules/IBidRule.h"
#include <memory>
#include <vector>

namespace auction::domain {

class BidValidator {
public:
    BidValidator() = default;

    void addRule(std::unique_ptr<IBidRule> rule);

    void validate(const PhienDauGia& auction,
                  const NguoiDung& bidder,
                  Money amount,
                  common::SteadyTime now) const;

    [[nodiscard]] static BidValidator createDefault(bool enableNotAlreadyLeading = false);

private:
    std::vector<std::unique_ptr<IBidRule>> rules_;
};

} // namespace auction::domain
