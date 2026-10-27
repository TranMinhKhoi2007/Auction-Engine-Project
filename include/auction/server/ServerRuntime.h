#pragma once
#include "auction/application/AuctionEngine.h"
#include "auction/infrastructure/BidHistoryLogObserver.h"
#include "auction/infrastructure_qt/QtEventLoopDriver.h"
#include "auction/server/ServerConfig.h"
namespace auction::server {
// Composition shared by the headless executable and network fixtures, never by the client.
class ServerRuntime {
public:
    explicit ServerRuntime(const std::string &logDirectory,ServerConfig config={},std::shared_ptr<domain::IPasswordHasher> hasher={});
    ~ServerRuntime();
    application::AuctionEngine &engine() { return application::AuctionEngine::instance(); }
    domain::ILogger &logger() { return *logger_; }
    domain::ILogger &security() { return *security_; }
    infrastructure::EventLoop &eventLoop() {return *loop_;}
    application::SessionService &sessions() {return *sessions_;}
    application::LoginThrottle &throttle() {return *throttle_;}
    const domain::PasswordHash &dummyHash() const {return dummyHash_;}
    std::shared_ptr<const domain::IPasswordHasher> passwordHasher() const {return passwordHasher_;}
    const ServerConfig &config() const {return config_;}
private:
    ServerConfig config_;
    std::shared_ptr<domain::IPasswordHasher> passwordHasher_;
    std::shared_ptr<domain::ITokenGenerator> tokens_;
    std::unique_ptr<application::SessionService> sessions_;
    std::unique_ptr<application::LoginThrottle> throttle_;
    domain::PasswordHash dummyHash_;
    std::shared_ptr<domain::ILogger> security_;
    std::shared_ptr<domain::IClock> clock_;
    std::shared_ptr<domain::ILogger> logger_;
    std::shared_ptr<infrastructure::EventLoop> loop_;
    std::shared_ptr<application::AuctionService> service_;
    std::shared_ptr<infrastructure::BidHistoryLogObserver> history_;
    std::shared_ptr<domain::IAuctionObserver> refunds_;
    std::unique_ptr<infrastructure_qt::QtEventLoopDriver> driver_;
};
}
