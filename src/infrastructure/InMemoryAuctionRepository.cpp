#include "auction/infrastructure/InMemoryAuctionRepository.h"
#include "auction/domain/entities/PhienDauGia.h"

namespace auction::infrastructure {

void InMemoryAuctionRepository::save(std::shared_ptr<domain::PhienDauGia> auction) {
    if (!auction) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auctions_[auction->id()] = std::move(auction);
}

bool InMemoryAuctionRepository::remove(common::AuctionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return auctions_.erase(id) > 0;
}

std::shared_ptr<domain::PhienDauGia> InMemoryAuctionRepository::findById(common::AuctionId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = auctions_.find(id);
    if (it != auctions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<domain::PhienDauGia>> InMemoryAuctionRepository::findAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<domain::PhienDauGia>> list;
    list.reserve(auctions_.size());
    for (const auto& [_, auction] : auctions_) {
        list.push_back(auction);
    }
    return list;
}

} // namespace auction::infrastructure
