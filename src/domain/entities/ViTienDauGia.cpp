#include "auction/domain/entities/ViTienDauGia.h"
#include "auction/domain/exceptions/AuctionException.h"

namespace auction::domain {

ViTienDauGia::ViTienDauGia(Money initialBalance) : available_(initialBalance) {}

void ViTienDauGia::topUp(Money amount) {
    available_ += amount;
}

void ViTienDauGia::hold(common::AuctionId auctionId, Money amount) {
    if (amount <= Money::zero()) {
        throw ValidationException("Khoản tiền ký quỹ phải lớn hơn 0");
    }
    if (holds_.find(auctionId) != holds_.end()) {
        throw ValidationException("Phiên đấu giá này đã được ký quỹ trước đó");
    }
    if (available_ < amount) {
        throw InsufficientBalanceException(amount.amount(), available_.amount());
    }

    available_ -= amount;
    holds_.emplace(auctionId, amount);
}

Money ViTienDauGia::release(common::AuctionId auctionId) noexcept {
    auto it = holds_.find(auctionId);
    if (it == holds_.end()) {
        return Money::zero();
    }
    Money held = it->second;
    holds_.erase(it);
    // Cộng lại vào available_, không thể ném exception vì held vốn được trừ từ available_ trước đó
    try {
        available_ += held;
    } catch (...) {
        // noexcept guarantee
    }
    return held;
}

bool ViTienDauGia::canSettle(common::AuctionId auctionId, Money price) const noexcept {
    Money held = heldFor(auctionId);
    return (available_.amount() + held.amount()) >= price.amount();
}

void ViTienDauGia::settle(common::AuctionId auctionId, Money price) {
    Money held = heldFor(auctionId);
    if ((available_.amount() + held.amount()) < price.amount()) {
        throw InsufficientBalanceException(price.amount(), available_.amount() + held.amount());
    }

    // Xóa khoản ký quỹ
    holds_.erase(auctionId);

    if (price >= held) {
        Money remainder = price - held;
        available_ -= remainder;
    } else {
        // Trường hợp hiếm hoi cọc > giá (hoàn phần dư)
        Money excess = held - price;
        available_ += excess;
    }
}

Money ViTienDauGia::forfeit(common::AuctionId auctionId) {
    auto it = holds_.find(auctionId);
    if (it == holds_.end()) {
        return Money::zero();
    }
    Money forfeited = it->second;
    holds_.erase(it);
    return forfeited;
}

void ViTienDauGia::receive(Money amount) {
    available_ += amount;
}

Money ViTienDauGia::heldFor(common::AuctionId auctionId) const noexcept {
    auto it = holds_.find(auctionId);
    if (it != holds_.end()) {
        return it->second;
    }
    return Money::zero();
}

Money ViTienDauGia::totalHeld() const noexcept {
    int64_t sum = 0;
    for (const auto& [_, amt] : holds_) {
        sum += amt.amount();
    }
    return Money(sum);
}

Money ViTienDauGia::total() const noexcept {
    return Money(available_.amount() + totalHeld().amount());
}

} // namespace auction::domain
