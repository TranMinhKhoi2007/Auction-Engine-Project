#pragma once

#include "auction/domain/values/PasswordHash.h"
#include <string_view>

namespace auction::domain {

class IPasswordHasher {
public:
    // Stateless/thread-safe implementations only. Worker threads receive values
    // and must never access a repository, engine, session or socket.
    virtual ~IPasswordHasher() = default;
    [[nodiscard]] virtual PasswordHash hash(std::string_view plain) const = 0;
    [[nodiscard]] virtual bool verify(std::string_view plain, const PasswordHash& stored) const = 0;
};

} // namespace auction::domain
