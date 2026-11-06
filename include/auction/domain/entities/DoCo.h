#pragma once

#include "auction/domain/entities/SanPhamDauGia.h"

namespace auction::domain {

class DoCo final : public SanPhamDauGia {
public:
    DoCo(common::ProductId id, std::string name, std::string description, common::UserId sellerId,
         int eraCentury, std::string origin, std::string preservationState, bool isAppraised);

    [[nodiscard]] std::string category() const override {
        return "Đồ cổ";
    }
    [[nodiscard]] std::string describe() const override;

    [[nodiscard]] int eraCentury() const noexcept {
        return eraCentury_;
    }
    [[nodiscard]] const std::string& origin() const noexcept {
        return origin_;
    }
    [[nodiscard]] const std::string& preservationState() const noexcept {
        return preservationState_;
    }
    [[nodiscard]] bool isAppraised() const noexcept {
        return isAppraised_;
    }

private:
    int eraCentury_;
    std::string origin_;
    std::string preservationState_;
    bool isAppraised_;
};

} // namespace auction::domain
