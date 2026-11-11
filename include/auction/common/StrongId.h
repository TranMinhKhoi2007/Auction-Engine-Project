#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>

namespace auction::common {

template <typename Tag, typename Rep = uint64_t>
class StrongId {
public:
    using UnderlyingType = Rep;

    constexpr StrongId() noexcept : value_(0) {}
    constexpr explicit StrongId(Rep value) noexcept : value_(value) {}

    [[nodiscard]] constexpr Rep value() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr auto operator<=>(const StrongId& other) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const StrongId& other) const noexcept = default;

    [[nodiscard]] std::string toString() const {
        return std::to_string(value_);
    }

    friend std::ostream& operator<<(std::ostream& os, const StrongId& id) {
        return os << id.value_;
    }

private:
    Rep value_{0};
};

struct AuctionTag {};
struct UserTag {};
struct ProductTag {};
struct BidTag {};
struct EventTag {};

using AuctionId = StrongId<AuctionTag, uint64_t>;
using UserId = StrongId<UserTag, uint64_t>;
using ProductId = StrongId<ProductTag, uint64_t>;
using BidId = StrongId<BidTag, uint64_t>;
using EventId = StrongId<EventTag, uint64_t>;

} // namespace auction::common

namespace std {
template <typename Tag, typename Rep>
struct hash<auction::common::StrongId<Tag, Rep>> {
    size_t operator()(const auction::common::StrongId<Tag, Rep>& id) const noexcept {
        return std::hash<Rep>{}(id.value());
    }
};
} // namespace std
