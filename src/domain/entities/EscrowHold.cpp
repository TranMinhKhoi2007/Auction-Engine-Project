#include "auction/domain/entities/EscrowHold.h"

namespace auction::domain {

EscrowHold::EscrowHold(ViTienDauGia& wallet, common::AuctionId auctionId, Money amount)
    : wallet_(&wallet), auctionId_(auctionId), committed_(false) {
    wallet_->hold(auctionId_, amount);
}

EscrowHold::~EscrowHold() noexcept {
    if (!committed_ && wallet_ != nullptr) {
        static_cast<void>(wallet_->release(auctionId_));
    }
}

EscrowHold::EscrowHold(EscrowHold&& other) noexcept
    : wallet_(other.wallet_), auctionId_(other.auctionId_), committed_(other.committed_) {
    other.wallet_ = nullptr;
    other.committed_ = true;
}

EscrowHold& EscrowHold::operator=(EscrowHold&& other) noexcept {
    if (this != &other) {
        if (!committed_ && wallet_ != nullptr) {
            static_cast<void>(wallet_->release(auctionId_));
        }
        wallet_ = other.wallet_;
        auctionId_ = other.auctionId_;
        committed_ = other.committed_;
        other.wallet_ = nullptr;
        other.committed_ = true;
    }
    return *this;
}

void EscrowHold::commit() noexcept {
    committed_ = true;
}

} // namespace auction::domain
