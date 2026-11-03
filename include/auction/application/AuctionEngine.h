#pragma once

#include "auction/application/AuctionScheduler.h"
#include "auction/application/AuctionService.h"
#include "auction/application/BiddingService.h"
#include "auction/application/Commands.h"
#include "auction/application/Dtos.h"
#include "auction/application/AccountService.h"
#include "auction/domain/ports/IAuctionRepository.h"
#include "auction/domain/ports/IClock.h"
#include "auction/domain/ports/ILogger.h"
#include "auction/domain/ports/IUserRepository.h"
#include "auction/domain/ports/IPasswordHasher.h"
#include "auction/domain/ports/IUnitOfWork.h"
#include "auction/infrastructure/EventLoop.h"
#include <deque>
#include <memory>
#include <optional>
#include <vector>

namespace auction::application {

struct EngineDependencies {
    std::shared_ptr<domain::IClock> clock;
    std::shared_ptr<domain::ILogger> logger;
    std::shared_ptr<domain::IAuctionRepository> auctionRepo;
    std::shared_ptr<domain::IUserRepository> userRepo;
    std::shared_ptr<infrastructure::EventLoop> eventLoop;
    std::shared_ptr<AuctionScheduler> scheduler;
    std::shared_ptr<AuctionService> auctionService;
    std::shared_ptr<BiddingService> biddingService;
    std::shared_ptr<domain::IPasswordHasher> passwordHasher;
    std::shared_ptr<domain::IUnitOfWork> unitOfWork{};
};

class AuctionEngine {
public:
    AuctionEngine(const AuctionEngine&) = delete;
    AuctionEngine& operator=(const AuctionEngine&) = delete;
    AuctionEngine(AuctionEngine&&) = delete;
    AuctionEngine& operator=(AuctionEngine&&) = delete;

    static void initialize(EngineDependencies&& deps);
    static AuctionEngine& instance();
    static void shutdown();

#ifdef AUCTION_TESTING
    static void resetForTesting();
#endif

    // Commands (Lệnh)
    UserDto registerUser(RegisterUserCommand command);
    UserDto authenticate(common::UserId id, std::string password);
    AccountDto authenticate(std::string email, std::string password);
    RegistrationDraft validateRegistration(std::string_view email,std::string_view name,std::string_view password) const;
    AccountDto completeRegistration(const RegistrationDraft &,domain::PasswordHash);
    [[nodiscard]] std::optional<CredentialRecord> findCredentials(const domain::Email &) const;
    [[nodiscard]] AccountDto accountOf(common::UserId) const;
    void updatePasswordHash(common::UserId,domain::PasswordHash);
    void topUp(common::UserId userId, domain::Money amount);
    common::AuctionId createAuction(const CreateAuctionCommand& command);
    void approveAuction(common::AuctionId id, common::UserId actingUserId);
    void startAuction(common::AuctionId id);
    void closeAuction(common::AuctionId id);
    void recoverAuctions();
    void restoreSchedules();
    void closeAuctionEarly(common::AuctionId id, common::UserId actingUserId);
    void cancelAuction(common::AuctionId id, common::UserId actingUserId, const std::string& reason);
    void deleteAuction(common::AuctionId id, common::UserId actingUserId);
    void followAuction(common::AuctionId id, common::UserId userId);
    void unfollowAuction(common::AuctionId id, common::UserId userId);
    void joinAuction(common::AuctionId id, common::UserId userId);
    void placeBid(common::AuctionId id, common::UserId userId, domain::Money amount);
    void addGlobalObserver(std::weak_ptr<domain::IAuctionObserver> observer);

    // Queries (Truy vấn trả DTO)
    [[nodiscard]] std::vector<UserDto> listUsers() const;
    [[nodiscard]] std::optional<UserDto> getUser(common::UserId id) const;
    [[nodiscard]] std::vector<AuctionSummaryDto> listAuctions() const;
    [[nodiscard]] std::optional<AuctionDetailDto> getAuctionDetail(common::AuctionId id) const;
    [[nodiscard]] std::optional<std::string> getAuctionImage(common::AuctionId id) const;
    [[nodiscard]] std::optional<WalletDto> getWallet(common::UserId id) const;
    [[nodiscard]] std::deque<domain::AuctionEvent> getInbox(common::UserId id) const;

    [[nodiscard]] const domain::IClock& clock() const;

private:
    void mutate(domain::MutationContext,const std::function<void()> &);
    explicit AuctionEngine(EngineDependencies&& deps);

    EngineDependencies deps_;
    std::unique_ptr<AccountService> accounts_;

    static std::unique_ptr<AuctionEngine> instance_;
};

} // namespace auction::application
