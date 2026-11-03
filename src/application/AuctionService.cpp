#include "auction/domain/exceptions/AuthorizationException.h"
#include "auction/application/AuctionService.h"
#include "auction/application/PhienDauGiaBuilder.h"
#include "auction/application/ProductFactory.h"
#include "auction/domain/entities/EscrowHold.h"
#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/exceptions/AuctionException.h"
#include <map>
#include <type_traits>
#include <exception>

namespace auction::application {

AuctionService::AuctionService(domain::IAuctionRepository& auctionRepo,
                               domain::IUserRepository& userRepo,
                               AuctionScheduler& scheduler,
                               std::shared_ptr<ICommissionPolicy> commissionPolicy,
                               const domain::IClock& clock)
    : auctionRepo_(auctionRepo),
      userRepo_(userRepo),
      scheduler_(scheduler),
      commissionPolicy_(std::move(commissionPolicy)),
      clock_(clock) {
    for(const auto &a:auctionRepo_.findAll()) {
        nextAuctionId_=std::max(nextAuctionId_,a->id().value()+1);
        nextProductId_=std::max(nextProductId_,a->item().id().value()+1);
    }
}

common::AuctionId AuctionService::createAuction(const CreateAuctionCommand& command) {
    auto seller = userRepo_.findById(command.sellerId);
    if (!seller) {
        throw domain::EntityNotFoundException("Người dùng (Người bán)", command.sellerId.value());
    }


    common::AuctionId auctionId{nextAuctionId_++};
    common::ProductId productId{nextProductId_++};

    auto product = ProductFactory::createProduct(productId, command.productCommand);
    product->setImageData(command.imageData);

    PhienDauGiaBuilder builder;
    builder.setId(auctionId)
        .setItem(std::move(product))
        .setSellerId(command.sellerId)
        .setStartingPrice(command.startingPrice)
        .setMinIncrement(command.minIncrement)
        .setDepositAmount(command.depositAmount)
        .setSchedule(command.startDelay, command.duration);

    auto auction = builder.build();

    for (const auto& obs : globalObservers_) {
        auction->publisher().subscribe(obs);
    }

    std::shared_ptr<domain::PhienDauGia> sharedAuction = std::move(auction);
    auctionRepo_.save(sharedAuction);
    seller->addAuction(auctionId);
    seller->follow(auctionId);
    sharedAuction->publisher().subscribe(seller);
    sharedAuction->publisher().publish(domain::StateChangedEvent{auctionId, "", "Chờ duyệt", clock_.wallNow()});

    return auctionId;
}

void AuctionService::approveAuction(common::AuctionId id, common::UserId actingUserId) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }

    if (actingUserId != auction->sellerId()) {
        throw domain::AuthorizationException("Chỉ chủ phiên mới được duyệt phiên.");
    }

    auction->approve(clock_.now(), clock_.wallNow());
    scheduler_.scheduleAuction(auction);
}

void AuctionService::startAuction(common::AuctionId id) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }
    auction->start();
}

void AuctionService::joinAuction(common::AuctionId id, common::UserId userId) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }

    auto user = userRepo_.findById(userId);
    if (!user) {
        throw domain::EntityNotFoundException("Người dùng", userId.value());
    }

    if (userId == auction->sellerId()) {
        throw domain::AuthorizationException("Người bán không thể tham gia đặt cọc cho phiên của chính mình");
    }

    auto participant = user;

    if (auction->hasJoined(userId)) {
        throw domain::ValidationException("Bạn đã tham gia đặt cọc cho phiên đấu giá này rồi");
    }

    // RAII EscrowHold: Tự động hoàn cọc nếu bước sau thất bại
    domain::EscrowHold hold(participant->wallet(), id, auction->depositAmount());

    // Thêm người tham gia vào phiên (kiểm tra trạng thái hợp lệ SapDienRa hoặc DangDienRa)
    auction->join(userId);

    // Thành công thì commit
    hold.commit();

    // Tự động follow và đăng ký observer
    participant->follow(id);
    auction->publisher().subscribe(participant);

    auction->publisher().publish(domain::ParticipantJoinedEvent{
        .auctionId = id,
        .userId = userId,
        .deposit = auction->depositAmount(),
        .timestamp = clock_.wallNow()
    });
}

void AuctionService::closeAuction(common::AuctionId id) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }
    if (auction->stateName() != "Đang diễn ra") {
        return;
    }
    finalizeAuction(*auction, false);
}

void AuctionService::closeAuctionEarly(common::AuctionId id, common::UserId actingUserId) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }
    if (actingUserId != auction->sellerId()) {
        throw domain::AuthorizationException("Chỉ người bán mới được chốt phiên.");
    }
    if (auction->stateName() != "Đang diễn ra") {
        throw domain::ValidationException("Chỉ chốt được phiên đang diễn ra.");
    }
    if (!auction->highestBid()) {
        throw domain::ValidationException("Chưa có lượt đặt giá nào để chốt.");
    }
    finalizeAuction(*auction, true);
}

void AuctionService::finalizeAuction(domain::PhienDauGia& auction, bool closedEarly) {
    // Stage the original settlement on value copies. Any arithmetic/policy/allocation
    // failure leaves the actual wallets, state and scheduled events untouched.
    struct WalletUpdate {
        std::shared_ptr<domain::NguoiDung> user;
        domain::ViTienDauGia wallet;
    };
    std::map<common::UserId, WalletUpdate> updates;
    auto stagedWallet = [&updates](const std::shared_ptr<domain::NguoiDung>& user)
        -> domain::ViTienDauGia& {
        auto [it, inserted] = updates.try_emplace(user->id(), user, user->wallet());
        (void)inserted;
        return it->second.wallet;
    };
    const auto id = auction.id();
    const auto seller = userRepo_.findById(auction.sellerId());
    const auto highest = auction.highestBid();
    auto outcome = domain::AuctionOutcome::NoBids;
    std::optional<common::UserId> winnerId;
    auto finalPrice = domain::Money::zero();
    if (highest) {
        winnerId = highest->bidderId();
        finalPrice = highest->amount();
        const auto winner = userRepo_.findById(*winnerId);
        if (winner && stagedWallet(winner).canSettle(id, finalPrice)) {
            stagedWallet(winner).settle(id, finalPrice);
            const auto commission = commissionPolicy_ ? commissionPolicy_->commission(finalPrice)
                                                      : domain::Money::zero();
            const auto netProceeds = finalPrice - commission;
            if (seller) {
                stagedWallet(seller).receive(netProceeds);
            }
            outcome = domain::AuctionOutcome::Sold;
        } else {
            if (winner) {
                const auto forfeited = stagedWallet(winner).forfeit(id);
                if (seller) {
                    stagedWallet(seller).receive(forfeited);
                }
            }
            outcome = domain::AuctionOutcome::PaymentDefault;
        }
    }
    for (auto participantId : auction.participants()) {
        if (!winnerId || participantId != *winnerId) {
            if (auto user = userRepo_.findById(participantId)) {
                stagedWallet(user).release(id);
            }
        }
    }
    const domain::AuctionClosedEvent event{id, outcome, winnerId, finalPrice,
                                           clock_.wallNow(), closedEarly};
    static_assert(std::is_nothrow_move_assignable_v<domain::ViTienDauGia>);

    // First mutation freezes bidding. Only this auction's scheduled events are cancelled.
    auction.close();
    std::exception_ptr cancellationError;
    try {
        scheduler_.cancelAuctionEvents(id);
    } catch (...) {
        // Even cancellation failure must not leave CLOSED with partial finances.
        // Any surviving close callback is harmless because closeAuction checks state.
        cancellationError = std::current_exception();
    }
    for (auto& [userId, update] : updates) {
        (void)userId;
        update.user->wallet() = std::move(update.wallet);
    }
    auction.setOutcome(outcome);
    auction.setClosedEarly(closedEarly);
    auction.publisher().publish(event);
    if (cancellationError) {
        std::rethrow_exception(cancellationError);
    }
}

void AuctionService::cancelAuction(common::AuctionId id, common::UserId actingUserId, const std::string& reason) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }

    if (actingUserId != auction->sellerId()) {
        throw domain::AuthorizationException("Chỉ chủ phiên mới được hủy phiên.");
    }

    scheduler_.cancelAuctionEvents(id);
    auction->cancel(reason);

    // Hoàn cọc cho toàn bộ người tham gia
    for (auto pId : auction->participants()) {
        auto u = userRepo_.findById(pId);
        if (u) {
            u->wallet().release(id);
        }
    }

    auction->publisher().publish(domain::AuctionCancelledEvent{
        .auctionId = id,
        .reason = reason,
        .timestamp = clock_.wallNow()
    });
}

void AuctionService::deleteAuction(common::AuctionId id, common::UserId actingUserId) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }

    if (actingUserId != auction->sellerId()) {
        throw domain::AuthorizationException("Chỉ người tạo phiên (chủ sở hữu) mới có quyền xóa phiên đấu giá này.");
    }

    scheduler_.cancelAuctionEvents(id);
    for (auto pId : auction->participants()) {
        auto u = userRepo_.findById(pId);
        if (u) {
            u->wallet().release(id);
        }
    }

    for (auto &u : userRepo_.findAll()) {
        u->unfollow(id);
        u->removeAuction(id);
        u->removeAuctionEvents(id);
    }

    for (auto &obs : globalObservers_) {
        if (auto p = obs.lock()) {
            p->onAuctionEvent(domain::AuctionCancelledEvent{
                .auctionId = id,
                .reason = "Phiên đấu giá đã được xóa khỏi hệ thống.",
                .timestamp = clock_.wallNow()
            });
        }
    }

    auctionRepo_.remove(id);
}

void AuctionService::followAuction(common::AuctionId id, common::UserId userId) {
    auto auction = auctionRepo_.findById(id);
    if (!auction) {
        throw domain::EntityNotFoundException("Phiên đấu giá", id.value());
    }
    auto user = userRepo_.findById(userId);
    if (!user) {
        throw domain::EntityNotFoundException("Người dùng", userId.value());
    }
    auto participant = user;
    if (participant) {
        participant->follow(id);
        auction->publisher().subscribe(participant);
    }
}

void AuctionService::unfollowAuction(common::AuctionId id, common::UserId userId) {
    auto user = userRepo_.findById(userId);
    if (user) {
        auto participant = user;
        if (participant) {
            participant->unfollow(id);
        }
    }
}

void AuctionService::addGlobalObserver(std::weak_ptr<domain::IAuctionObserver> observer) {
    globalObservers_.push_back(observer);
    for (auto& auction : auctionRepo_.findAll()) {
        auction->publisher().subscribe(observer);
    }
}

} // namespace auction::application
