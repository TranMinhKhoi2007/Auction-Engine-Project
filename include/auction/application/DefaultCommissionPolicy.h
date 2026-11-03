#pragma once

#include "auction/application/ICommissionPolicy.h"

namespace auction::application {

class DefaultCommissionPolicy final : public ICommissionPolicy {
public:
    explicit DefaultCommissionPolicy(int percent = 0) : percent_(percent) {}

    [[nodiscard]] domain::Money commission(domain::Money price) const override {
        if (percent_ <= 0) {
            return domain::Money::zero();
        }
        return price.percentOf(percent_);
    }

private:
    int percent_{0};
};

} // namespace auction::application
