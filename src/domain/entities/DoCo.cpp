#include "auction/domain/entities/DoCo.h"
#include <sstream>

namespace auction::domain {

DoCo::DoCo(common::ProductId id, std::string name, std::string description, common::UserId sellerId,
           int eraCentury, std::string origin, std::string preservationState, bool isAppraised)
    : SanPhamDauGia(id, std::move(name), std::move(description), sellerId),
      eraCentury_(eraCentury),
      origin_(std::move(origin)),
      preservationState_(std::move(preservationState)),
      isAppraised_(isAppraised) {}

std::string DoCo::describe() const {
    std::ostringstream oss;
    oss << name() << " (" << description() << ") - Đồ cổ thế kỷ " << eraCentury_
        << ", Xuất xứ: " << origin_ << ", Tình trạng: " << preservationState_
        << ", Giám định: " << (isAppraised_ ? "Đã giám định" : "Chưa giám định");
    return oss.str();
}

} // namespace auction::domain
