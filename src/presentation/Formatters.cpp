#include "auction/presentation/Formatters.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace auction::presentation {

QString Formatters::formatMoney(domain::Money money) {
    return QString::fromUtf8(money.toString().c_str());
}

QString Formatters::formatTimeRemaining(common::Millis remaining) {
    int64_t totalSec = remaining.count() / 1000;
    if (totalSec <= 0) {
        return QString::fromUtf8("00:00");
    }

    int64_t hours = totalSec / 3600;
    int64_t mins = (totalSec % 3600) / 60;
    int64_t secs = totalSec % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

QString Formatters::formatWallTime(common::WallTime wallTime) {
    auto nowTimeT = std::chrono::system_clock::to_time_t(wallTime);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &nowTimeT);
#else
    localtime_r(&nowTimeT, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%d/%m/%Y %H:%M:%S");
    return QString::fromStdString(oss.str());
}

QString Formatters::formatAuctionOutcome(domain::AuctionOutcome outcome) {
    return QString::fromUtf8(domain::toVietnameseString(outcome).c_str());
}

QString Formatters::formatBidRejectReason(const QString &reason) {
    if(reason=="AuctionNotActive") return "Phiên chưa mở hoặc đã đóng, không thể đặt giá.";
    if(reason=="BelowStartingPrice") return "Giá đặt thấp hơn giá khởi điểm.";
    if(reason=="BelowMinimumIncrement") return "Giá đặt chưa đủ bước giá tối thiểu.";
    if(reason=="InsufficientFunds") return "Không đủ số dư hợp lệ để đặt mức giá này.";
    if(reason=="SellerCannotBid") return "Người bán không thể đặt giá phiên của mình.";
    if(reason=="AuctionExpired") return "Phiên đã hết giờ.";
    if(reason=="DepositNotPaid") return "Bạn cần đặt cọc để tham gia trước khi đặt giá.";
    if(reason=="AlreadyLeading") return "Bạn đang dẫn đầu. Chờ người khác đặt giá.";
    return "Không thể đặt giá.";
}

QString Formatters::formatEventSummary(const domain::AuctionEvent &event,
                                       common::UserId currentUserId) {
    return std::visit(
        [currentUserId](const auto &e) -> QString {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, domain::ParticipantJoinedEvent>) {
                return QString::fromUtf8("Phiên #%1: Người dùng U-%2 đã tham gia (cọc %3)")
                    .arg(e.auctionId.value())
                    .arg(e.userId.value())
                    .arg(formatMoney(e.deposit));
            } else if constexpr (std::is_same_v<T, domain::BidPlacedEvent>) {
                return QString::fromUtf8("Phiên #%1: U-%2 đặt giá mới %3")
                    .arg(e.auctionId.value())
                    .arg(e.bidderId.value())
                    .arg(formatMoney(e.amount));
            } else if constexpr (std::is_same_v<T, domain::OutbidEvent>) {
                if (e.previousLeader == currentUserId) {
                    return QString::fromUtf8("Bạn đã bị vượt giá tại Phiên #%1! Giá mới là %2")
                        .arg(e.auctionId.value())
                        .arg(formatMoney(e.newAmount));
                }
                return QString::fromUtf8("Phiên #%1: Người dùng U-%2 vừa bị vượt giá bởi mức %3")
                    .arg(e.auctionId.value())
                    .arg(e.previousLeader.value())
                    .arg(formatMoney(e.newAmount));
            } else if constexpr (std::is_same_v<T, domain::StateChangedEvent>) {
                return QString::fromUtf8("Phiên #%1: Chuyển trạng thái từ '%2' sang '%3'")
                    .arg(e.auctionId.value())
                    .arg(QString::fromUtf8(e.fromState.c_str()))
                    .arg(QString::fromUtf8(e.toState.c_str()));
            } else if constexpr (std::is_same_v<T, domain::CountdownEvent>) {
                return QString::fromUtf8("Phiên #%1: Chỉ còn %2 giây!")
                    .arg(e.auctionId.value())
                    .arg(e.secondsRemaining);
            } else if constexpr (std::is_same_v<T, domain::AuctionClosedEvent>) {
                if (e.outcome == domain::AuctionOutcome::Sold && e.winnerId.has_value()) {
                    return (e.closedEarly ? QString("Chốt sớm · ") : QString()) +
                        QString::fromUtf8("Phiên #%1 KẾT THÚC: U-%2 trúng thầu với giá %3")
                        .arg(e.auctionId.value())
                        .arg(e.winnerId->value())
                        .arg(formatMoney(e.finalPrice));
                }
                return (e.closedEarly ? QString("Chốt sớm · ") : QString()) +
                    QString::fromUtf8("Phiên #%1 KẾT THÚC: Kết quả '%2'")
                    .arg(e.auctionId.value())
                    .arg(formatAuctionOutcome(e.outcome));
            } else if constexpr (std::is_same_v<T, domain::AuctionCancelledEvent>) {
                return QString::fromUtf8("Phiên #%1 ĐÃ BỊ HỦY: %2")
                    .arg(e.auctionId.value())
                    .arg(QString::fromUtf8(e.reason.c_str()));
            }
            return QString::fromUtf8("Sự kiện đấu giá");
        },
        event);
}

} // namespace auction::presentation
