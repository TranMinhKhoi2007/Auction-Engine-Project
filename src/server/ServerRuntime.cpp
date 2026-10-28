#include "auction/server/ServerRuntime.h"
#include "auction/application/DefaultCommissionPolicy.h"
#include "auction/infrastructure/ConsoleLogger.h"
#include "auction/infrastructure/CompositeLogger.h"
#include "auction/infrastructure/FileLogger.h"
#include "auction/infrastructure/InMemoryAuctionRepository.h"
#include "auction/infrastructure/InMemoryUserRepository.h"
#include "auction/infrastructure/SystemClock.h"
#include "auction/infrastructure_qt/Pbkdf2PasswordHasher.h"
#include "auction/infrastructure_qt/OsTokenGenerator.h"
#include "auction/infrastructure_qt/SqliteStore.h"
#include "auction/domain/values/SecretBuffer.h"
#include <filesystem>
namespace auction::server {
namespace {
// Audit actual completed settlements; it never changes wallets or business rules.
class RefundAudit final : public domain::IAuctionObserver {
public:
    explicit RefundAudit(infrastructure::BidHistoryLogObserver &history):history_(history) {}
    void onAuctionEvent(const domain::AuctionEvent &e) override {
        std::optional<common::AuctionId> id; std::optional<common::UserId> winner;
        if(const auto *closed=std::get_if<domain::AuctionClosedEvent>(&e)) {id=closed->auctionId;winner=closed->winnerId;}
        if(const auto *cancelled=std::get_if<domain::AuctionCancelledEvent>(&e)) id=cancelled->auctionId;
        if(!id) return;
        const auto dto=application::AuctionEngine::instance().getAuctionDetail(*id);
        if(!dto) return;
        for(auto user:dto->participants) if(user!=winner) history_.logRefund(*id,user,dto->depositAmount);
        if(dto->outcome==domain::AuctionOutcome::PaymentDefault && winner) history_.logForfeit(*id,*winner,dto->depositAmount);
    }
private:
    infrastructure::BidHistoryLogObserver &history_;
};
}
ServerRuntime::ServerRuntime(const std::string &logDirectory,ServerConfig config,std::shared_ptr<domain::IPasswordHasher> hasher):config_(std::move(config)) {
    std::filesystem::create_directories(logDirectory);
    clock_=std::make_shared<infrastructure::SystemClock>();
    security_=std::make_shared<infrastructure::FileLogger>(logDirectory+"/security.log");
    passwordHasher_=hasher?std::move(hasher):std::make_shared<infrastructure_qt::Pbkdf2PasswordHasher>(config_.passwordHashIterations);
    tokens_=std::make_shared<infrastructure_qt::OsTokenGenerator>();
    sessions_=std::make_unique<application::SessionService>(*clock_,*tokens_,config_.sessions);
    throttle_=std::make_unique<application::LoginThrottle>(*clock_,config_.throttle);
    {
        auto seed=tokens_->generate(domain::kSessionTokenBytes);domain::SecretBuffer wipe(seed);
        dummyHash_=passwordHasher_->hash(seed); // Before accepting connections/start of business activity.
    }
    auto logs=std::make_shared<infrastructure::CompositeLogger>();
    logs->addLogger(std::make_shared<infrastructure::FileLogger>(logDirectory+"/app.log"));
    logs->addLogger(std::make_shared<infrastructure::ConsoleLogger>()); logger_=logs;
    std::shared_ptr<domain::IAuctionRepository> auctions;
    std::shared_ptr<domain::IUserRepository> users;
    std::shared_ptr<domain::IUnitOfWork> unitOfWork;
    if(config_.databasePath.empty()) {
        auctions=std::make_shared<infrastructure::InMemoryAuctionRepository>();
        users=std::make_shared<infrastructure::InMemoryUserRepository>();
    } else {
        auto store=std::make_shared<infrastructure_qt::SqliteStore>(config_.databasePath,*clock_,[this]{
            if(driver_)driver_->stop();
            logger_->error("SQLITE_WRITE_FAILED: business timers stopped; database requires attention");
        });
        auctions=std::make_shared<infrastructure_qt::SqlAuctionRepository>(store);
        users=std::make_shared<infrastructure_qt::SqlUserRepository>(store);unitOfWork=store;
    }
    loop_=std::make_shared<infrastructure::EventLoop>(*clock_,logger_.get());
    auto scheduler=std::make_shared<application::AuctionScheduler>(*loop_,*clock_,
        [this](common::AuctionId id){engine().startAuction(id);},[this](common::AuctionId id){engine().closeAuction(id);},unitOfWork);
    service_=std::make_shared<application::AuctionService>(*auctions,*users,*scheduler,std::make_shared<application::DefaultCommissionPolicy>(0),*clock_);
    auto bidding=std::make_shared<application::BiddingService>(*auctions,*users,*clock_,domain::BidValidator::createDefault());
    application::AuctionEngine::initialize({clock_,logger_,auctions,users,loop_,scheduler,service_,bidding,passwordHasher_,unitOfWork});
    try {
    history_=std::make_shared<infrastructure::BidHistoryLogObserver>(logDirectory+"/auction_history.log",users.get());
    refunds_=std::make_shared<RefundAudit>(*history_);
    engine().addGlobalObserver(history_); engine().addGlobalObserver(refunds_);
    engine().recoverAuctions();
    driver_=std::make_unique<infrastructure_qt::QtEventLoopDriver>(*loop_,*clock_); driver_->start();
    } catch(...) {
        application::AuctionEngine::shutdown();throw;
    }
}
ServerRuntime::~ServerRuntime() {
    driver_.reset();
    // Destroy SQL connections while QCoreApplication and its plugins still exist.
    application::AuctionEngine::shutdown();
}
}
