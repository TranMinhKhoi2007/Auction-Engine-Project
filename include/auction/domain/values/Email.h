#pragma once
#include "auction/domain/exceptions/AuctionException.h"
#include <functional>
#include <string>
#include <string_view>
namespace auction::domain {
class AccountValidationException final : public ValidationException {
public:
    AccountValidationException(std::string reason,const std::string &message)
        :ValidationException(message),reason_(std::move(reason)) {}
    [[nodiscard]] const std::string &reason() const noexcept {return reason_;}
private:
    std::string reason_;
};
namespace account_text {
[[nodiscard]] std::u32string decode(std::string_view);
[[nodiscard]] std::string encode(std::u32string_view);
[[nodiscard]] bool space(char32_t);
[[nodiscard]] bool control(char32_t);
[[nodiscard]] std::u32string trim(std::u32string);
[[nodiscard]] std::u32string lower(std::u32string_view);
[[nodiscard]] std::u32string fold(std::u32string_view);
}
class Email final {
public:
    [[nodiscard]] static Email parse(std::string_view);
    [[nodiscard]] const std::string &value() const noexcept {return value_;}
    [[nodiscard]] std::string masked() const;
    [[nodiscard]] bool operator==(const Email &) const noexcept=default;
private:
    explicit Email(std::string value):value_(std::move(value)) {}
    std::string value_;
};
}
template<> struct std::hash<auction::domain::Email> {
    size_t operator()(const auction::domain::Email &email) const noexcept {
        return std::hash<std::string>{}(email.value());
    }
};
