#pragma once
#include "auction/domain/ports/ITokenGenerator.h"
#include <vector>
namespace auction::infrastructure_qt {
class OsTokenGenerator final : public domain::ITokenGenerator {
public:
    [[nodiscard]] std::string generate(size_t bytes) const override;
    [[nodiscard]] Digest digest(std::string_view token) const override;
    [[nodiscard]] static std::vector<uint8_t> randomBytes(size_t count);
};
}
