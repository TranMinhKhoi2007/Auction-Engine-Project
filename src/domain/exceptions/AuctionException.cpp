#include "auction/domain/exceptions/AuctionException.h"
#include <sstream>

namespace auction::domain {

std::string toVietnameseString(BidRejectReason reason) {
    switch (reason) {
    case BidRejectReason::AuctionNotActive:
        return "Phiên đấu giá chưa mở hoặc đã kết thúc";
    case BidRejectReason::BelowStartingPrice:
        return "Giá đặt thấp hơn giá khởi điểm";
    case BidRejectReason::BelowMinimumIncrement:
        return "Giá đặt không đáp ứng bước giá tối thiểu";
    case BidRejectReason::InsufficientFunds:
        return "Số dư khả dụng không đủ khả năng thanh toán";
    case BidRejectReason::SellerCannotBid:
        return "Người bán không được phép tham gia đặt giá cho phiên của mình";
    case BidRejectReason::AuctionExpired:
        return "Phiên đấu giá đã hết thời gian";
    case BidRejectReason::DepositNotPaid:
        return "Bạn chưa đặt cọc ký quỹ để tham gia phiên này";
    case BidRejectReason::AlreadyLeading:
        return "Bạn hiện đã là người trả giá cao nhất";
    }
    return "Lượt đặt giá không hợp lệ";
}

InvalidBidException::InvalidBidException(BidRejectReason reason, const std::string& message)
    : AuctionException(message.empty() ? toVietnameseString(reason) : message), reason_(reason) {}

InvalidStateTransitionException::InvalidStateTransitionException(const std::string& currentState,
                                                                 const std::string& action)
    : AuctionException("Không thể thực hiện hành động '" + action + "' ở trạng thái '" + currentState + "'"),
      currentState_(currentState),
      action_(action) {}

InsufficientBalanceException::InsufficientBalanceException(int64_t required, int64_t available)
    : AuctionException("Số dư không đủ: yêu cầu " + std::to_string(required) +
                       " VND, khả dụng " + std::to_string(available) + " VND"),
      required_(required),
      available_(available) {}

EntityNotFoundException::EntityNotFoundException(const std::string& entityType, uint64_t id)
    : AuctionException("Không tìm thấy " + entityType + " với ID " + std::to_string(id)),
      entityType_(entityType),
      id_(id) {}

} // namespace auction::domain
