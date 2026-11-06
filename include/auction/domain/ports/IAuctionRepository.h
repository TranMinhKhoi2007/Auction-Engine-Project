#pragma once

#include "auction/common/StrongId.h"
#include <memory>
#include <vector>

namespace auction::domain {

class PhienDauGia;

class IAuctionRepository {
public:
    virtual ~IAuctionRepository() = default;

    virtual void save(std::shared_ptr<PhienDauGia> auction) = 0;
    virtual bool remove(common::AuctionId id) = 0;
    [[nodiscard]] virtual std::shared_ptr<PhienDauGia> findById(common::AuctionId id) const = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<PhienDauGia>> findAll() const = 0;
};

} // namespace auction::domain
