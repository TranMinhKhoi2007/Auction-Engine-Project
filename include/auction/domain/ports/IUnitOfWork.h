#pragma once
#include "auction/common/StrongId.h"
#include "auction/domain/events/IAuctionObserver.h"
#include <functional>
#include <memory>
#include <string>
namespace auction::domain {
struct MutationContext {
    std::string operation;
    common::UserId actor{};
    common::AuctionId auction{};
};
// A durable boundary for a complete business command, including wallet/inbox effects.
class IUnitOfWork {
public:
    virtual ~IUnitOfWork()=default;
    [[nodiscard]] virtual bool isHealthy() const noexcept {return true;}
    virtual void execute(const MutationContext &,const std::function<void()> &)=0;
    virtual std::weak_ptr<IAuctionObserver> afterCommit(std::weak_ptr<IAuctionObserver>)=0;
};
}
