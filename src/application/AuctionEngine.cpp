#include "auction/application/AuctionEngine.h"
#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/values/SecretBuffer.h"
#include <cctype>
#include <algorithm>
#include "auction/domain/exceptions/AuctionException.h"
#include <stdexcept>

namespace auction::application {

std::unique_ptr<AuctionEngine> AuctionEngine::instance_ = nullptr;

AuctionEngine::AuctionEngine(EngineDependencies&& deps) : deps_(std::move(deps)),
    accounts_(std::make_unique<AccountService>(*deps_.userRepo)) {}

void AuctionEngine::initialize(EngineDependencies&& deps) {
    instance_.reset(new AuctionEngine(std::move(deps)));
}

AuctionEngine& AuctionEngine::instance() {
    if (!instance_) {
        throw std::logic_error("AuctionEngine chưa được khởi tạo! Hãy gọi AuctionEngine::initialize trước.");
    }
    return *instance_;
}

#ifdef AUCTION_TESTING
void AuctionEngine::resetForTesting() {
    shutdown();
}
#endif
void AuctionEngine::shutdown() {instance_.reset();}

UserDto AuctionEngine::registerUser(RegisterUserCommand command) {
    domain::SecretBuffer wipe(command.password);
    const auto draft=validateRegistration(command.email,command.name,command.password);
    if(!deps_.passwordHasher)throw std::logic_error("Chưa cấu hình bộ băm mật khẩu");
    const auto account=completeRegistration(draft,deps_.passwordHasher->hash(command.password));
    if(deps_.logger)deps_.logger->info("USER_REGISTERED USER="+account.id.toString());
    return *getUser(account.id);
}
RegistrationDraft AuctionEngine::validateRegistration(std::string_view email,std::string_view name,std::string_view password) const {
    return accounts_->validateRegistration(email,name,password);
}
AccountDto AuctionEngine::completeRegistration(const RegistrationDraft &draft,domain::PasswordHash hash) {
    AccountDto result{};
    mutate({"REGISTER",{},{}},[&]{result=accounts_->completeRegistration(draft,std::move(hash));});
    return result;
}
std::optional<CredentialRecord> AuctionEngine::findCredentials(const domain::Email &email) const {return accounts_->findCredentials(email);}
AccountDto AuctionEngine::accountOf(common::UserId id) const {return accounts_->accountOf(id);}
void AuctionEngine::updatePasswordHash(common::UserId id,domain::PasswordHash hash) {
    mutate({"PASSWORD_REHASH",id,{}},[&]{accounts_->updatePasswordHash(id,std::move(hash));});
}
AccountDto AuctionEngine::authenticate(std::string email,std::string password) {
    domain::SecretBuffer wipe(password);
    const auto record=findCredentials(domain::Email::parse(email));
    if(!deps_.passwordHasher)throw std::logic_error("Chưa cấu hình bộ băm mật khẩu");
    if(!record || !deps_.passwordHasher->verify(password,record->passwordHash))
        throw domain::AuthenticationException("Email hoặc mật khẩu không đúng");
    return accountOf(record->userId);
}

UserDto AuctionEngine::authenticate(common::UserId id, std::string password) {
    domain::SecretBuffer wipe(password);
    auto user = deps_.userRepo->findById(id);
    if (!deps_.passwordHasher) {
        throw std::logic_error("Chưa cấu hình bộ băm mật khẩu");
    }
    if (!user || !deps_.passwordHasher->verify(password, user->passwordHash())) {
        if (deps_.logger) {
            deps_.logger->warn("LOGIN_FAILED USER=" + id.toString());
        }
        throw domain::AuthenticationException("Sai mật khẩu");
    }
    if (deps_.logger) {
        deps_.logger->info("LOGIN_OK USER=" + id.toString());
    }
    return *getUser(id);
}

void AuctionEngine::topUp(common::UserId userId, domain::Money amount) {
    mutate({"TOP_UP",userId,{}},[&]{
    auto user = deps_.userRepo->findById(userId);
    if (!user) {
        throw domain::EntityNotFoundException("Người dùng", userId.value());
    }
    user->wallet().topUp(amount);
    });
}

common::AuctionId AuctionEngine::createAuction(const CreateAuctionCommand& command) {
    common::AuctionId result{};
    mutate({"CREATE",command.sellerId,{}},[&]{result=deps_.auctionService->createAuction(command);});
    return result;
}

void AuctionEngine::approveAuction(common::AuctionId id, common::UserId actingUserId) {
    mutate({"APPROVE",actingUserId,id},[&]{deps_.auctionService->approveAuction(id, actingUserId);});
}

void AuctionEngine::closeAuctionEarly(common::AuctionId id, common::UserId actingUserId) {
    mutate({"CLOSE_EARLY",actingUserId,id},[&]{deps_.auctionService->closeAuctionEarly(id, actingUserId);});
}

void AuctionEngine::startAuction(common::AuctionId id) {
    mutate({"START",{},id},[&]{deps_.auctionService->startAuction(id);});
}

void AuctionEngine::cancelAuction(common::AuctionId id, common::UserId actingUserId, const std::string& reason) {
    mutate({"CANCEL",actingUserId,id},[&]{deps_.auctionService->cancelAuction(id, actingUserId, reason);});
}

void AuctionEngine::deleteAuction(common::AuctionId id, common::UserId actingUserId) {
    mutate({"DELETE_AUCTION",actingUserId,id},[&]{deps_.auctionService->deleteAuction(id, actingUserId);});
}

void AuctionEngine::followAuction(common::AuctionId id, common::UserId userId) {
    mutate({"FOLLOW",userId,id},[&]{deps_.auctionService->followAuction(id, userId);});
}

void AuctionEngine::unfollowAuction(common::AuctionId id, common::UserId userId) {
    mutate({"UNFOLLOW",userId,id},[&]{deps_.auctionService->unfollowAuction(id, userId);});
}

void AuctionEngine::joinAuction(common::AuctionId id, common::UserId userId) {
    mutate({"HOLD",userId,id},[&]{deps_.auctionService->joinAuction(id, userId);});
}

void AuctionEngine::placeBid(common::AuctionId id, common::UserId userId, domain::Money amount) {
    mutate({"BID",userId,id},[&]{deps_.biddingService->placeBid(id, userId, amount);});
}

void AuctionEngine::addGlobalObserver(std::weak_ptr<domain::IAuctionObserver> observer) {
    if(deps_.unitOfWork)observer=deps_.unitOfWork->afterCommit(observer);
    deps_.auctionService->addGlobalObserver(observer);
}

void AuctionEngine::closeAuction(common::AuctionId id) {
    mutate({"CLOSE",{},id},[&]{deps_.auctionService->closeAuction(id);});
}
void AuctionEngine::mutate(domain::MutationContext context,const std::function<void()> &action) {
    if(!deps_.unitOfWork) {action();return;}
    try {deps_.unitOfWork->execute(context,action);}
    catch(...) {
        if(deps_.unitOfWork->isHealthy())restoreSchedules();
        else for(const auto &a:deps_.auctionRepo->findAll())deps_.scheduler->cancelAuctionEvents(a->id());
        throw;
    }
}
void AuctionEngine::restoreSchedules() {
    for(const auto &a:deps_.auctionRepo->findAll()) {
        deps_.scheduler->cancelAuctionEvents(a->id());
        if(a->stateName()=="Sắp diễn ra" || a->stateName()=="Đang diễn ra")deps_.scheduler->scheduleAuction(a);
    }
}
void AuctionEngine::recoverAuctions() {
    struct Due {common::WallTime time;common::AuctionId id;bool close;};
    std::vector<Due> due;
    const auto now=deps_.clock->wallNow();
    const auto users=deps_.userRepo->findAll();
    for(const auto &a:deps_.auctionRepo->findAll()) {
        for(const auto &u:users)
            if(u->isFollowing(a->id()) || a->hasJoined(u->id()) || a->sellerId()==u->id())a->publisher().subscribe(u);
        const bool scheduled=a->stateName()=="Sắp diễn ra";
        const bool active=a->stateName()=="Đang diễn ra";
        if(scheduled && a->startWallTime() && *a->startWallTime()<=now)due.push_back({*a->startWallTime(),a->id(),false});
        if((scheduled || active) && a->endWallTime() && *a->endWallTime()<=now)due.push_back({*a->endWallTime(),a->id(),true});
    }
    std::sort(due.begin(),due.end(),[](const Due &a,const Due &b){
        if(a.time!=b.time)return a.time<b.time;
        if(a.close!=b.close)return !a.close;
        return a.id<b.id;
    });
    for(const auto &e:due) {if(e.close)closeAuction(e.id);else startAuction(e.id);}
    restoreSchedules();
}

const domain::IClock& AuctionEngine::clock() const {
    return *deps_.clock;
}

std::vector<UserDto> AuctionEngine::listUsers() const {
    auto allUsers = deps_.userRepo->findAll();
    std::vector<UserDto> dtos;
    dtos.reserve(allUsers.size());

    for (const auto& u : allUsers) {
        dtos.push_back(UserDto{
            .id = u->id(),
            .name = u->name(),
            .availableBalance = u->wallet().available(),
            .heldBalance = u->wallet().totalHeld(),
            .totalBalance = u->wallet().total()
        });
    }
    return dtos;
}

std::optional<UserDto> AuctionEngine::getUser(common::UserId id) const {
    auto u = deps_.userRepo->findById(id);
    if (!u) {
        return std::nullopt;
    }
    return UserDto{
        .id = u->id(),
        .name = u->name(),
        .availableBalance = u->wallet().available(),
        .heldBalance = u->wallet().totalHeld(),
        .totalBalance = u->wallet().total()
    };
}

std::vector<AuctionSummaryDto> AuctionEngine::listAuctions() const {
    auto allAuctions = deps_.auctionRepo->findAll();
    std::vector<AuctionSummaryDto> dtos;
    dtos.reserve(allAuctions.size());

    auto now = deps_.clock->now();
    for (const auto& a : allAuctions) {
        dtos.push_back(AuctionSummaryDto{
            .id = a->id(),
            .productName = a->item().name(),
            .category = a->item().category(),
            .state = a->stateName(),
            .currentPrice = a->currentPrice(),
            .remainingTime = a->remainingTime(now)
        });
    }
    return dtos;
}

std::optional<AuctionDetailDto> AuctionEngine::getAuctionDetail(common::AuctionId id) const {
    auto a = deps_.auctionRepo->findById(id);
    if (!a) {
        return std::nullopt;
    }

    std::string sellerName = "Không rõ";
    if (auto s = deps_.userRepo->findById(a->sellerId())) {
        sellerName = s->name();
    }

    std::optional<common::UserId> leaderId;
    std::string leaderName = "Chưa có";
    auto highest = a->highestBid();
    if (highest.has_value()) {
        leaderId = highest->bidderId();
        if (auto leader = deps_.userRepo->findById(*leaderId)) {
            leaderName = leader->name();
        }
    }

    std::vector<common::UserId> participants;
    for (auto pId : a->participants()) {
        participants.push_back(pId);
    }

    std::vector<domain::LuotTraGia> bids;
    for (const auto& b : a->bidHistory()) {
        bids.push_back(b);
    }

    return AuctionDetailDto{
        .id = a->id(),
        .productId = a->item().id(),
        .productName = a->item().name(),
        .category = a->item().category(),
        .productDescription = a->item().describe(),
        .sellerId = a->sellerId(),
        .sellerName = sellerName,
        .startingPrice = a->startingPrice(),
        .minIncrement = a->minIncrement(),
        .depositAmount = a->depositAmount(),
        .currentPrice = a->currentPrice(),
        .minimumNextBid = a->minimumNextBid(),
        .leaderId = leaderId,
        .leaderName = leaderName,
        .state = a->stateName(),
        .outcome = a->outcome(),
        .closedEarly = a->closedEarly(),
        .startWallTime = a->startWallTime(),
        .endWallTime = a->endWallTime(),
        .startDelayMs = a->startDelay(),
        .durationMs = a->duration(),
        .remainingTime = a->remainingTime(deps_.clock->now()),
        .participants = std::move(participants),
        .bidHistory = std::move(bids),
        .hasImage = !a->item().imageData().empty()
    };
}

std::optional<std::string> AuctionEngine::getAuctionImage(common::AuctionId id) const {
    const auto auction=deps_.auctionRepo->findById(id);
    if(!auction)return std::nullopt;
    return auction->item().imageData();
}

std::optional<WalletDto> AuctionEngine::getWallet(common::UserId id) const {
    auto u = deps_.userRepo->findById(id);
    if (!u) {
        return std::nullopt;
    }

    std::vector<WalletHoldEntry> holdEntries;
    for (const auto& [aucId, amt] : u->wallet().holds()) {
        std::string title = "Phiên #" + aucId.toString();
        if (auto a = deps_.auctionRepo->findById(aucId)) {
            title = a->item().name();
        }
        holdEntries.push_back(WalletHoldEntry{
            .auctionId = aucId,
            .auctionTitle = title,
            .amount = amt
        });
    }

    return WalletDto{
        .available = u->wallet().available(),
        .totalHeld = u->wallet().totalHeld(),
        .total = u->wallet().total(),
        .holds = std::move(holdEntries)
    };
}

std::deque<domain::AuctionEvent> AuctionEngine::getInbox(common::UserId id) const {
    auto u = deps_.userRepo->findById(id);
    if (!u) {
        return {};
    }
    return u->inbox();
}

} // namespace auction::application
