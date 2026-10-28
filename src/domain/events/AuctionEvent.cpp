#include "auction/domain/events/AuctionEvent.h"

namespace auction::domain {

std::string toVietnameseString(AuctionOutcome outcome) {
    switch (outcome) {
    case AuctionOutcome::None:
        return "Chưa xác định";
    case AuctionOutcome::Sold:
        return "Thành công (Đã bán)";
    case AuctionOutcome::NoBids:
        return "Không có người đặt giá";
    case AuctionOutcome::PaymentDefault:
        return "Người thắng không đủ tiền thanh toán";
    case AuctionOutcome::Cancelled:
        return "Đã bị hủy";
    }
    return "Không rõ";
}

} // namespace auction::domain
