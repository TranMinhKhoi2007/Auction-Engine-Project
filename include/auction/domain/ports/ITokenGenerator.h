#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
namespace auction::domain {
class ITokenGenerator {
public:
    using Digest=std::array<uint8_t,32>;
    virtual ~ITokenGenerator()=default;
    [[nodiscard]] virtual std::string generate(size_t bytes) const=0;
    // Digest also crosses a port so SessionService remains Qt-free and does not
    // implement cryptography. Adapters must use SHA-256 from their crypto library.
    [[nodiscard]] virtual Digest digest(std::string_view token) const=0;
};
}
