#include "auction/domain/rules/BidValidator.h"
#include "auction/domain/rules/AuctionActiveRule.h"
#include "auction/domain/rules/DepositPaidRule.h"
#include "auction/domain/rules/MinimumIncrementRule.h"
#include "auction/domain/rules/NotAlreadyLeadingRule.h"
#include "auction/domain/rules/NotSellerRule.h"
#include "auction/domain/rules/SufficientFundsRule.h"

namespace auction::domain {

void BidValidator::addRule(std::unique_ptr<IBidRule> rule) {
    if (rule) {
        rules_.push_back(std::move(rule));
    }
}

void BidValidator::validate(const PhienDauGia& auction,
                            const NguoiDung& bidder,
                            Money amount,
                            common::SteadyTime now) const {
    for (const auto& rule : rules_) {
        rule->validate(auction, bidder, amount, now);
    }
}

BidValidator BidValidator::createDefault(bool enableNotAlreadyLeading) {
    BidValidator validator;
    validator.addRule(std::make_unique<AuctionActiveRule>());
    validator.addRule(std::make_unique<NotSellerRule>());
    validator.addRule(std::make_unique<DepositPaidRule>());
    validator.addRule(std::make_unique<MinimumIncrementRule>());
    validator.addRule(std::make_unique<SufficientFundsRule>());
    if (enableNotAlreadyLeading) {
        validator.addRule(std::make_unique<NotAlreadyLeadingRule>());
    }
    return validator;
}

} // namespace auction::domain
