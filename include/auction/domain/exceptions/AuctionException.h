#pragma once

#include "auction/common/StrongId.h"
#include <cstdint>
#include <stdexcept>
#include <string>

namespace auction::domain {

enum class BidRejectReason {
    AuctionNotActive,
    BelowStartingPrice,
    BelowMinimumIncrement,
    InsufficientFunds,
    SellerCannotBid,
    AuctionExpired,
    DepositNotPaid,
    AlreadyLeading
};

[[nodiscard]] std::string toVietnameseString(BidRejectReason reason);

class AuctionException : public std::runtime_error {
public:
    explicit AuctionException(const std::string& message) : std::runtime_error(message) {}
};

class InvalidBidException : public AuctionException {
public:
    InvalidBidException(BidRejectReason reason, const std::string& message);

    [[nodiscard]] BidRejectReason reason() const noexcept {
        return reason_;
    }

private:
    BidRejectReason reason_;
};

class InvalidStateTransitionException : public AuctionException {
public:
    InvalidStateTransitionException(const std::string& currentState, const std::string& action);

    [[nodiscard]] const std::string& currentState() const noexcept {
        return currentState_;
    }
    [[nodiscard]] const std::string& action() const noexcept {
        return action_;
    }

private:
    std::string currentState_;
    std::string action_;
};

class InsufficientBalanceException : public AuctionException {
public:
    InsufficientBalanceException(int64_t required, int64_t available);

    [[nodiscard]] int64_t required() const noexcept {
        return required_;
    }
    [[nodiscard]] int64_t available() const noexcept {
        return available_;
    }

private:
    int64_t required_;
    int64_t available_;
};

class EntityNotFoundException : public AuctionException {
public:
    EntityNotFoundException(const std::string& entityType, uint64_t id);

    [[nodiscard]] const std::string& entityType() const noexcept {
        return entityType_;
    }
    [[nodiscard]] uint64_t id() const noexcept {
        return id_;
    }

private:
    std::string entityType_;
    uint64_t id_;
};

class AuthenticationException : public AuctionException {
public:
    explicit AuthenticationException(const std::string& message) : AuctionException(message) {}
};

class ValidationException : public AuctionException {
public:
    explicit ValidationException(const std::string& message) : AuctionException(message) {}
};

} // namespace auction::domain
