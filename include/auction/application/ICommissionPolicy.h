#pragma once

#include "auction/domain/values/Money.h"

namespace auction::application {

class ICommissionPolicy {
public:
    virtual ~ICommissionPolicy() = default;
    [[nodiscard]] virtual domain::Money commission(domain::Money price) const = 0;
};

} // namespace auction::application
