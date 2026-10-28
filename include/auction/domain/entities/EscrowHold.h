#pragma once

#include "auction/common/StrongId.h"
#include "auction/domain/entities/ViTienDauGia.h"
#include "auction/domain/values/Money.h"

namespace auction::domain {

class EscrowHold {
public:
    EscrowHold(ViTienDauGia& wallet, common::AuctionId auctionId, Money amount);
    ~EscrowHold() noexcept;

    EscrowHold(const EscrowHold&) = delete;
    EscrowHold& operator=(const EscrowHold&) = delete;

    EscrowHold(EscrowHold&& other) noexcept;
    EscrowHold& operator=(EscrowHold&& other) noexcept;

    void commit() noexcept;

private:
    ViTienDauGia* wallet_{nullptr};
    common::AuctionId auctionId_{};
    bool committed_{false};
};

} // namespace auction::domain
