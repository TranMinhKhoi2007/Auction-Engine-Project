#include "auction/infrastructure/BidHistoryLogObserver.h"
#include "auction/domain/entities/NguoiDung.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace auction::infrastructure {

namespace {
std::string formatIsoTimestamp(common::WallTime wallTime) {
    auto nowTimeT = std::chrono::system_clock::to_time_t(wallTime);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(wallTime.time_since_epoch()) % 1000;

    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &nowTimeT);
#else
    localtime_r(&nowTimeT, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}
} // namespace

BidHistoryLogObserver::BidHistoryLogObserver(const std::string& logFilePath,
                                             const domain::IUserRepository* userRepo)
    : userRepo_(userRepo) {
    std::filesystem::path path(logFilePath);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    fileStream_.open(logFilePath, std::ios::out | std::ios::app);
}

BidHistoryLogObserver::~BidHistoryLogObserver() {
    if (fileStream_.is_open()) {
        fileStream_.flush();
        fileStream_.close();
    }
}

std::string BidHistoryLogObserver::getUserDisplay(common::UserId id) {
    std::string str = "U-" + id.toString();
    if (userRepo_ != nullptr) {
        auto u = userRepo_->findById(id);
        if (u) {
            str += " (" + u->name() + ")";
        }
    }
    return str;
}

void BidHistoryLogObserver::writeLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!fileStream_.is_open()) {
        return;
    }
    fileStream_ << line << "\n";
    fileStream_.flush();
}

void BidHistoryLogObserver::onAuctionEvent(const domain::AuctionEvent& event) {
    std::visit(
        [this](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, domain::ParticipantJoinedEvent>) {
                std::ostringstream oss;
                oss << formatIsoTimestamp(e.timestamp) << " | AUCTION=A-" << e.auctionId.toString()
                    << " | JOIN   | USER=" << getUserDisplay(e.userId)
                    << " | DEPOSIT=" << e.deposit.toString() << " | RESULT=HELD";
                writeLine(oss.str());
            } else if constexpr (std::is_same_v<T, domain::BidPlacedEvent>) {
                std::ostringstream oss;
                oss << formatIsoTimestamp(e.timestamp) << " | AUCTION=A-" << e.auctionId.toString()
                    << " | BID    | USER=" << getUserDisplay(e.bidderId)
                    << " | AMOUNT=" << e.amount.toString() << " | RESULT=ACCEPTED";
                writeLine(oss.str());
            } else if constexpr (std::is_same_v<T, domain::StateChangedEvent>) {
                std::ostringstream oss;
                oss << formatIsoTimestamp(e.timestamp) << " | AUCTION=A-" << e.auctionId.toString()
                    << " | STATE  | " << e.fromState << " -> " << e.toState;
                writeLine(oss.str());
            } else if constexpr (std::is_same_v<T, domain::AuctionClosedEvent>) {
                std::ostringstream oss;
                oss << formatIsoTimestamp(e.timestamp) << " | AUCTION=A-" << e.auctionId.toString()
                    << " | CLOSE  | OUTCOME=" << toVietnameseString(e.outcome);
                if (e.winnerId.has_value()) {
                    oss << " | WINNER=" << getUserDisplay(*e.winnerId)
                        << " | PRICE=" << e.finalPrice.toString();
                }
                if (e.closedEarly) {
                    oss << " | EARLY=true";
                }
                writeLine(oss.str());
            } else if constexpr (std::is_same_v<T, domain::AuctionCancelledEvent>) {
                std::ostringstream oss;
                oss << formatIsoTimestamp(e.timestamp) << " | AUCTION=A-" << e.auctionId.toString()
                    << " | CANCEL | REASON=" << e.reason;
                writeLine(oss.str());
            }
        },
        event);
}

void BidHistoryLogObserver::logRejectedBid(common::AuctionId auctionId,
                                           common::UserId userId,
                                           domain::Money amount,
                                           domain::BidRejectReason reason) {
    std::ostringstream oss;
    oss << formatIsoTimestamp(std::chrono::system_clock::now()) << " | AUCTION=A-" << auctionId.toString()
        << " | BID    | USER=" << getUserDisplay(userId)
        << " | AMOUNT=" << amount.toString() << " | RESULT=REJECTED | REASON="
        << toVietnameseString(reason);
    writeLine(oss.str());
}

void BidHistoryLogObserver::logRefund(common::AuctionId auctionId, common::UserId userId, domain::Money amount) {
    std::ostringstream oss;
    oss << formatIsoTimestamp(std::chrono::system_clock::now()) << " | AUCTION=A-" << auctionId.toString()
        << " | REFUND | USER=" << getUserDisplay(userId)
        << " | AMOUNT=" << amount.toString();
    writeLine(oss.str());
}

void BidHistoryLogObserver::logForfeit(common::AuctionId auctionId, common::UserId userId, domain::Money amount) {
    std::ostringstream oss;
    oss << formatIsoTimestamp(std::chrono::system_clock::now()) << " | AUCTION=A-" << auctionId.toString()
        << " | FORFEIT| USER=" << getUserDisplay(userId)
        << " | AMOUNT=" << amount.toString();
    writeLine(oss.str());
}

} // namespace auction::infrastructure
