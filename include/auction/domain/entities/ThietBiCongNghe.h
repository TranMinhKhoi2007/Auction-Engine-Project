#pragma once

#include "auction/domain/entities/SanPhamDauGia.h"

namespace auction::domain {

class ThietBiCongNghe final : public SanPhamDauGia {
public:
    ThietBiCongNghe(common::ProductId id, std::string name, std::string description, common::UserId sellerId,
                    std::string brand, std::string model, int warrantyMonths, bool isNew);

    [[nodiscard]] std::string category() const override {
        return "Thiết bị công nghệ";
    }
    [[nodiscard]] std::string describe() const override;

    [[nodiscard]] const std::string& brand() const noexcept {
        return brand_;
    }
    [[nodiscard]] const std::string& model() const noexcept {
        return model_;
    }
    [[nodiscard]] int warrantyMonths() const noexcept {
        return warrantyMonths_;
    }
    [[nodiscard]] bool isNew() const noexcept {
        return isNew_;
    }

private:
    std::string brand_;
    std::string model_;
    int warrantyMonths_;
    bool isNew_;
};

} // namespace auction::domain
