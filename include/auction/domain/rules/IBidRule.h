#pragma once

#include "auction/common/TimeTypes.h"
#include "auction/domain/values/Money.h"

namespace auction::domain {

class PhienDauGia;
class NguoiDung;

class IBidRule {
public:
    virtual ~IBidRule() = default;

    virtual void validate(const PhienDauGia& auction,
                          const NguoiDung& bidder,
                          Money amount,
                          common::SteadyTime now) const = 0;
};

} // namespace auction::domain
