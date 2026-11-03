#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/entities/PhienDauGia.h"
#include "auction/domain/entities/SanPhamDauGia.h"
#include "auction/domain/ports/IClock.h"
#include "auction/domain/values/Money.h"
#include <memory>

namespace auction::application {

class PhienDauGiaBuilder {
public:
    PhienDauGiaBuilder() = default;

    PhienDauGiaBuilder& setId(common::AuctionId id);
    PhienDauGiaBuilder& setItem(std::unique_ptr<domain::SanPhamDauGia> item);
    PhienDauGiaBuilder& setSellerId(common::UserId sellerId);
    PhienDauGiaBuilder& setStartingPrice(domain::Money price);
    PhienDauGiaBuilder& setMinIncrement(domain::Money increment);
    PhienDauGiaBuilder& setDepositAmount(domain::Money deposit);
    PhienDauGiaBuilder& setSchedule(common::Millis startDelay, common::Millis duration);

    [[nodiscard]] std::unique_ptr<domain::PhienDauGia> build();

private:
    common::AuctionId id_{};
    std::unique_ptr<domain::SanPhamDauGia> item_;
    common::UserId sellerId_{};
    domain::Money startingPrice_{domain::Money::zero()};
    domain::Money minIncrement_{domain::Money::zero()};
    domain::Money depositAmount_{domain::Money::zero()};
    common::Millis startDelay_{};
    common::Millis duration_{};
};

} // namespace auction::application
