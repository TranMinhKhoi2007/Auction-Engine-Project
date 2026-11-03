#pragma once
#include "auction/domain/ports/IPasswordHasher.h"
#include "auction/domain/AuctionConfig.h"
#include <span>
namespace auction::infrastructure_qt {
class Pbkdf2PasswordHasher final : public domain::IPasswordHasher {
public:
    explicit Pbkdf2PasswordHasher(uint32_t iterations=domain::kPasswordHashIterations);
    [[nodiscard]] domain::PasswordHash hash(std::string_view plain) const override;
    [[nodiscard]] bool verify(std::string_view plain,const domain::PasswordHash &) const override;
    [[nodiscard]] static std::vector<uint8_t> derive(std::string_view plain,std::span<const uint8_t> salt,uint32_t iterations);
    [[nodiscard]] uint32_t iterations() const noexcept {return iterations_;}
private:
    const uint32_t iterations_;
};
}
