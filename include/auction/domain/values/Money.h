#pragma once

#include <compare>
#include <cstdint>
#include <iosfwd>
#include <string>

namespace auction::domain {

class Money {
public:
    constexpr Money() noexcept : amount_(0) {}
    explicit Money(int64_t amount);

    [[nodiscard]] constexpr int64_t amount() const noexcept {
        return amount_;
    }

    [[nodiscard]] Money percentOf(int percent) const;

    [[nodiscard]] Money operator+(const Money& other) const;
    [[nodiscard]] Money operator-(const Money& other) const;
    Money& operator+=(const Money& other);
    Money& operator-=(const Money& other);

    [[nodiscard]] constexpr bool operator==(const Money& other) const noexcept {
        return amount_ == other.amount_;
    }

    [[nodiscard]] constexpr auto operator<=>(const Money& other) const noexcept {
        return amount_ <=> other.amount_;
    }

    [[nodiscard]] constexpr bool operator<(const Money& other) const noexcept {
        return amount_ < other.amount_;
    }

    [[nodiscard]] constexpr bool operator>(const Money& other) const noexcept {
        return amount_ > other.amount_;
    }

    [[nodiscard]] constexpr bool operator<=(const Money& other) const noexcept {
        return amount_ <= other.amount_;
    }

    [[nodiscard]] constexpr bool operator>=(const Money& other) const noexcept {
        return amount_ >= other.amount_;
    }

    [[nodiscard]] std::string toString() const;

    friend std::ostream& operator<<(std::ostream& os, const Money& money);

    [[nodiscard]] static constexpr Money zero() noexcept {
        return Money();
    }

private:
    int64_t amount_{0};
};

} // namespace auction::domain
