#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/ImageLimits.h"
#include <stdexcept>
#include <string>

namespace auction::domain {

class SanPhamDauGia {
public:
    SanPhamDauGia(common::ProductId id, std::string name, std::string description, common::UserId sellerId)
        : id_(id), name_(std::move(name)), description_(std::move(description)), sellerId_(sellerId) {}

    virtual ~SanPhamDauGia() = default;

    SanPhamDauGia(const SanPhamDauGia&) = delete;
    SanPhamDauGia& operator=(const SanPhamDauGia&) = delete;
    SanPhamDauGia(SanPhamDauGia&&) noexcept = default;
    SanPhamDauGia& operator=(SanPhamDauGia&&) noexcept = default;

    [[nodiscard]] common::ProductId id() const noexcept {
        return id_;
    }
    [[nodiscard]] const std::string& name() const noexcept {
        return name_;
    }
    [[nodiscard]] const std::string& description() const noexcept {
        return description_;
    }
    [[nodiscard]] common::UserId sellerId() const noexcept {
        return sellerId_;
    }

    [[nodiscard]] virtual std::string category() const = 0;
    [[nodiscard]] virtual std::string describe() const = 0;
    [[nodiscard]] const std::string &imageData() const noexcept { return imageData_; }
    void setImageData(std::string data) {
        if(data.size()>common::kMaxProductImageBytes)throw std::invalid_argument("Ảnh sản phẩm vượt giới hạn dung lượng");
        imageData_=std::move(data);
    }

private:
    common::ProductId id_;
    std::string name_;
    std::string description_;
    common::UserId sellerId_;
    std::string imageData_;
};

} // namespace auction::domain
