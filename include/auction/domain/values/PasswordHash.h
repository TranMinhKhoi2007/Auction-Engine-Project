#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>
#include <vector>

namespace auction::domain {

struct PasswordHash {
    std::vector<uint8_t> salt;
    std::vector<uint8_t> hash;
    uint32_t iterations{};
    std::string algorithm{"pbkdf2-sha256"};
    [[nodiscard]] std::string encode() const;
    [[nodiscard]] static PasswordHash decode(std::string_view encoded);
    [[nodiscard]] bool needsRehash(uint32_t currentIterations) const noexcept {
        return algorithm!="pbkdf2-sha256" || iterations<currentIterations;
    }
};

class PasswordHashFormatError final : public std::invalid_argument {
public:
    PasswordHashFormatError():std::invalid_argument("Định dạng băm mật khẩu không hợp lệ") {}
};

} // namespace auction::domain
