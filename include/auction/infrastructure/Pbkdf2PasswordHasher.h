#pragma once

#include "auction/domain/ports/IPasswordHasher.h"
#include <array>

namespace auction::infrastructure {

// Windows CNG / OpenSSL implement the KDF and cryptographic random generator.
class Pbkdf2PasswordHasher final : public domain::IPasswordHasher {
public:
    [[nodiscard]] domain::PasswordHash hash(std::string_view plain) const override;
    [[nodiscard]] bool verify(std::string_view plain, const domain::PasswordHash& stored) const override;
    [[nodiscard]] static std::array<uint8_t, 32> sha256(std::string_view input);
};

} // namespace auction::infrastructure
