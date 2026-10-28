#pragma once

#include "auction/domain/entities/SanPhamDauGia.h"

namespace auction::domain {

class TranhNgheThuat final : public SanPhamDauGia {
public:
    TranhNgheThuat(common::ProductId id, std::string name, std::string description, common::UserId sellerId,
                   std::string artist, int yearCreated, std::string medium,
                   std::string dimensions, bool hasCertificate);

    [[nodiscard]] std::string category() const override {
        return "Tranh nghệ thuật";
    }
    [[nodiscard]] std::string describe() const override;

    [[nodiscard]] const std::string& artist() const noexcept {
        return artist_;
    }
    [[nodiscard]] int yearCreated() const noexcept {
        return yearCreated_;
    }
    [[nodiscard]] const std::string& medium() const noexcept {
        return medium_;
    }
    [[nodiscard]] const std::string& dimensions() const noexcept {
        return dimensions_;
    }
    [[nodiscard]] bool hasCertificate() const noexcept {
        return hasCertificate_;
    }

private:
    std::string artist_;
    int yearCreated_;
    std::string medium_;
    std::string dimensions_;
    bool hasCertificate_;
};

} // namespace auction::domain
