#include "auction/domain/values/Money.h"
#include "auction/domain/exceptions/AuctionException.h"
#include <algorithm>
#include <limits>
#include <ostream>

namespace auction::domain {

Money::Money(int64_t amount) : amount_(amount) {
    if (amount < 0) {
        throw ValidationException("Số tiền không được âm: " + std::to_string(amount));
    }
}

Money Money::percentOf(int percent) const {
    if (percent < 0) {
        throw ValidationException("Tỷ lệ phần trăm không được âm: " + std::to_string(percent));
    }
    if (percent == 0 || amount_ == 0) {
        return Money(0);
    }
    // Kiểm tra overflow khi nhân
    if (amount_ > std::numeric_limits<int64_t>::max() / percent) {
        throw ValidationException("Tràn số khi tính phần trăm số tiền");
    }
    int64_t product = amount_ * percent;
    return Money(product / 100);
}

Money Money::operator+(const Money& other) const {
    if (std::numeric_limits<int64_t>::max() - other.amount_ < amount_) {
        throw ValidationException("Tràn số khi cộng tiền");
    }
    return Money(amount_ + other.amount_);
}

Money Money::operator-(const Money& other) const {
    if (amount_ < other.amount_) {
        throw InsufficientBalanceException(other.amount_, amount_);
    }
    return Money(amount_ - other.amount_);
}

Money& Money::operator+=(const Money& other) {
    *this = *this + other;
    return *this;
}

Money& Money::operator-=(const Money& other) {
    *this = *this - other;
    return *this;
}

std::string Money::toString() const {
    std::string raw = std::to_string(amount_);
    std::string formatted;
    int count = 0;
    for (auto it = raw.rbegin(); it != raw.rend(); ++it) {
        if (count > 0 && count % 3 == 0) {
            formatted.push_back('.');
        }
        formatted.push_back(*it);
        ++count;
    }
    std::reverse(formatted.begin(), formatted.end());
    formatted += " VND";
    return formatted;
}

std::ostream& operator<<(std::ostream& os, const Money& money) {
    return os << money.toString();
}

} // namespace auction::domain
