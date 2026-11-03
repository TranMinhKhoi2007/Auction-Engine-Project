#include "auction/application/PhienDauGiaBuilder.h"
#include "auction/domain/AuctionConfig.h"
#include "auction/domain/exceptions/AuctionException.h"

namespace auction::application {

PhienDauGiaBuilder& PhienDauGiaBuilder::setId(common::AuctionId id) {
    id_ = id;
    return *this;
}

PhienDauGiaBuilder& PhienDauGiaBuilder::setItem(std::unique_ptr<domain::SanPhamDauGia> item) {
    item_ = std::move(item);
    return *this;
}

PhienDauGiaBuilder& PhienDauGiaBuilder::setSellerId(common::UserId sellerId) {
    sellerId_ = sellerId;
    return *this;
}

PhienDauGiaBuilder& PhienDauGiaBuilder::setStartingPrice(domain::Money price) {
    startingPrice_ = price;
    return *this;
}

PhienDauGiaBuilder& PhienDauGiaBuilder::setMinIncrement(domain::Money increment) {
    minIncrement_ = increment;
    return *this;
}

PhienDauGiaBuilder& PhienDauGiaBuilder::setDepositAmount(domain::Money deposit) {
    depositAmount_ = deposit;
    return *this;
}

PhienDauGiaBuilder& PhienDauGiaBuilder::setSchedule(common::Millis startDelay,
                                                   common::Millis duration) {
    startDelay_ = startDelay;
    duration_ = duration;
    return *this;
}

std::unique_ptr<domain::PhienDauGia> PhienDauGiaBuilder::build() {
    if (!item_) {
        throw domain::ValidationException("Phiên đấu giá phải có sản phẩm");
    }
    if (startingPrice_ <= domain::Money::zero()) {
        throw domain::ValidationException("Giá khởi điểm phải lớn hơn 0");
    }
    if (minIncrement_ <= domain::Money::zero()) {
        throw domain::ValidationException("Bước giá phải lớn hơn 0");
    }
    if (depositAmount_ <= domain::Money::zero() || depositAmount_ > startingPrice_) {
        throw domain::ValidationException("Mức ký quỹ phải lớn hơn 0 và nhỏ hơn hoặc bằng giá khởi điểm");
    }
    if (duration_ < domain::kMinAuctionDuration) {
        throw domain::ValidationException("Thời gian phiên đấu giá phải tối thiểu là 5 giây");
    }
    if (startDelay_ < domain::kMinStartLead) {
        throw domain::ValidationException("Phiên phải bắt đầu sau khi duyệt ít nhất 1 giây");
    }

    return std::make_unique<domain::PhienDauGia>(
        id_, std::move(item_), sellerId_, startingPrice_, minIncrement_, depositAmount_,
        startDelay_, duration_);
}

} // namespace auction::application
