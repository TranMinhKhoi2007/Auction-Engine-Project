#include "auction/application/LoginThrottle.h"
#include <algorithm>
#include <stdexcept>
namespace auction::application {
namespace {
void trim(std::deque<common::SteadyTime> &times,common::SteadyTime cutoff) {
    while(!times.empty() && times.front()<=cutoff)times.pop_front();
}
common::Millis remaining(common::SteadyTime deadline,common::SteadyTime now) {
    return std::max(common::Millis{0},std::chrono::ceil<common::Millis>(deadline-now));
}
}
LoginThrottle::LoginThrottle(const domain::IClock &clock,ThrottleConfig config):clock_(clock),config_(config) {
    if(config.emailFailures==0 || config.addressFailures==0 || config.registrationLimit==0 ||
       config.initialLock.count()<=0 || config.maximumLock<config.initialLock ||
       config.addressWindow.count()<=0 || config.registrationWindow.count()<=0)
        throw std::invalid_argument("Invalid throttle configuration");
}
ThrottleResult LoginThrottle::check(std::string_view email,std::string_view address) {
    const auto now=clock_.now();common::Millis retry{};
    if(const auto it=emails_.find(email);it!=emails_.end())retry=remaining(it->second.blockedUntil,now);
    if(const auto it=addresses_.find(address);it!=addresses_.end()) {
        trim(it->second,now-config_.addressWindow);
        if(it->second.size()>=config_.addressFailures)retry=std::max(retry,remaining(it->second.front()+config_.addressWindow,now));
    }
    return {retry};
}
void LoginThrottle::recordFailure(std::string_view email,std::string_view address) {
    const auto now=clock_.now();auto &state=emails_[std::string(email)];
    if(state.failures<config_.emailFailures)++state.failures;
    if(state.failures>=config_.emailFailures) {
        state.penalty=state.penalty.count()==0?config_.initialLock:std::min(config_.maximumLock,state.penalty*2);
        state.blockedUntil=now+state.penalty;
    }
    auto &attempts=addresses_[std::string(address)];trim(attempts,now-config_.addressWindow);attempts.push_back(now);
}
void LoginThrottle::recordSuccess(std::string_view email,std::string_view address) {
    if(const auto it=emails_.find(email);it!=emails_.end())emails_.erase(it);
    // A valid account cannot reset the address-wide failure quota of an attacker.
    if(const auto it=addresses_.find(address);it!=addresses_.end())trim(it->second,clock_.now()-config_.addressWindow);
}
ThrottleResult LoginThrottle::registrationAttempt(std::string_view address) {
    const auto now=clock_.now();auto &attempts=registrations_[std::string(address)];trim(attempts,now-config_.registrationWindow);
    if(attempts.size()>=config_.registrationLimit)return {remaining(attempts.front()+config_.registrationWindow,now)};
    attempts.push_back(now);return {};
}
}
