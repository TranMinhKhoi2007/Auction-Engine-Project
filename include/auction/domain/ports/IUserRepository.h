#pragma once

#include "auction/common/StrongId.h"
#include "auction/domain/values/Email.h"
#include <memory>
#include <vector>

namespace auction::domain {

class NguoiDung;

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual void save(std::shared_ptr<NguoiDung> user) = 0;
    [[nodiscard]] virtual std::shared_ptr<NguoiDung> findById(common::UserId id) const = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<NguoiDung>> findAll() const = 0;
    [[nodiscard]] virtual std::shared_ptr<NguoiDung> findByEmail(const Email &) const = 0;
    [[nodiscard]] virtual bool emailExists(const Email &) const = 0;
};

} // namespace auction::domain
