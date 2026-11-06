#include "auction/domain/entities/ThietBiCongNghe.h"
#include <sstream>

namespace auction::domain {

ThietBiCongNghe::ThietBiCongNghe(common::ProductId id, std::string name, std::string description,
                                 common::UserId sellerId, std::string brand, std::string model,
                                 int warrantyMonths, bool isNew)
    : SanPhamDauGia(id, std::move(name), std::move(description), sellerId),
      brand_(std::move(brand)),
      model_(std::move(model)),
      warrantyMonths_(warrantyMonths),
      isNew_(isNew) {}

std::string ThietBiCongNghe::describe() const {
    std::ostringstream oss;
    oss << name() << " (" << description() << ") - Hãng: " << brand_ << ", Model: " << model_
        << ", Tình trạng: " << (isNew_ ? "Mới 100%" : "Đã qua sử dụng")
        << ", Bảo hành: " << warrantyMonths_ << " tháng";
    return oss.str();
}

} // namespace auction::domain
