#pragma once

#include "auction/domain/ports/IUserRepository.h"
#include <mutex>
#include <unordered_map>

namespace auction::infrastructure {

class InMemoryUserRepository final : public domain::IUserRepository {
public:
    void save(std::shared_ptr<domain::NguoiDung> user) override;
    [[nodiscard]] std::shared_ptr<domain::NguoiDung> findById(common::UserId id) const override;
    [[nodiscard]] std::vector<std::shared_ptr<domain::NguoiDung>> findAll() const override;
    [[nodiscard]] std::shared_ptr<domain::NguoiDung> findByEmail(const domain::Email &) const override;
    [[nodiscard]] bool emailExists(const domain::Email &) const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<common::UserId, std::shared_ptr<domain::NguoiDung>> users_;
    std::unordered_map<domain::Email,common::UserId> emails_;
};

} // namespace auction::infrastructure
