#include "auction/domain/entities/TranhNgheThuat.h"
#include <sstream>

namespace auction::domain {

TranhNgheThuat::TranhNgheThuat(common::ProductId id, std::string name, std::string description,
                               common::UserId sellerId, std::string artist, int yearCreated,
                               std::string medium, std::string dimensions, bool hasCertificate)
    : SanPhamDauGia(id, std::move(name), std::move(description), sellerId),
      artist_(std::move(artist)),
      yearCreated_(yearCreated),
      medium_(std::move(medium)),
      dimensions_(std::move(dimensions)),
      hasCertificate_(hasCertificate) {}

std::string TranhNgheThuat::describe() const {
    std::ostringstream oss;
    oss << name() << " (" << description() << ") - Họa sĩ: " << artist_ << ", Sáng tác: " << yearCreated_
        << ", Chất liệu: " << medium_ << ", Kích thước: " << dimensions_
        << ", Chứng nhận: " << (hasCertificate_ ? "Có chứng thực" : "Chưa có chứng thực");
    return oss.str();
}

} // namespace auction::domain
