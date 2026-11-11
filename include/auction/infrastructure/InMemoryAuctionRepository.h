#pragma once

#include "auction/domain/ports/IAuctionRepository.h"
#include <mutex>
#include <unordered_map>

namespace auction::infrastructure {

class InMemoryAuctionRepository final : public domain::IAuctionRepository {
public:
    void save(std::shared_ptr<domain::PhienDauGia> auction) override;
    bool remove(common::AuctionId id) override;
    [[nodiscard]] std::shared_ptr<domain::PhienDauGia> findById(common::AuctionId id) const override;
    [[nodiscard]] std::vector<std::shared_ptr<domain::PhienDauGia>> findAll() const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<common::AuctionId, std::shared_ptr<domain::PhienDauGia>> auctions_;
};

} // namespace auction::infrastructure
