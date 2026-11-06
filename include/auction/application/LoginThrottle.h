#pragma once
#include "auction/domain/AuctionConfig.h"
#include "auction/domain/ports/IClock.h"
#include <deque>
#include <map>
#include <optional>
#include <string>
namespace auction::application {
struct ThrottleConfig {
    unsigned emailFailures{domain::kLoginEmailFailureLimit};
    common::Millis initialLock{domain::kLoginInitialLock},maximumLock{domain::kLoginMaximumLock};
    size_t addressFailures{domain::kLoginAddressFailureLimit},registrationLimit{domain::kRegistrationAddressLimit};
    common::Millis addressWindow{domain::kLoginAddressWindow},registrationWindow{domain::kRegistrationWindow};
};
struct ThrottleResult {
    common::Millis retryAfter{};
    [[nodiscard]] bool allowed() const noexcept {return retryAfter.count()==0;}
};
class LoginThrottle final {
public:
    explicit LoginThrottle(const domain::IClock &,ThrottleConfig={});
    [[nodiscard]] ThrottleResult check(std::string_view emailKey,std::string_view addressKey);
    void recordFailure(std::string_view emailKey,std::string_view addressKey);
    void recordSuccess(std::string_view emailKey,std::string_view addressKey);
    [[nodiscard]] ThrottleResult registrationAttempt(std::string_view addressKey);
private:
    struct EmailState {unsigned failures{};common::Millis penalty{};common::SteadyTime blockedUntil{};};
    const domain::IClock &clock_;
    ThrottleConfig config_;
    std::map<std::string,EmailState,std::less<>> emails_;
    std::map<std::string,std::deque<common::SteadyTime>,std::less<>> addresses_,registrations_;
};
}
