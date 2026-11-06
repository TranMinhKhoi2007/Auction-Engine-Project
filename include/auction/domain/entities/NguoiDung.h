#pragma once

#include "auction/common/StrongId.h"
#include "auction/domain/AuctionConfig.h"
#include "auction/domain/entities/ViTienDauGia.h"
#include "auction/domain/events/IAuctionObserver.h"
#include "auction/domain/values/PasswordHash.h"
#include "auction/domain/values/Email.h"
#include <deque>
#include <span>
#include <string>
#include <vector>

namespace auction::application { class AccountService; }
namespace auction::domain {

class NguoiDung final : public IAuctionObserver {
public:
    NguoiDung(common::UserId id, std::string name, PasswordHash passwordHash, Email email)
        : id_(id), name_(std::move(name)), passwordHash_(std::move(passwordHash)), email_(std::move(email)) {}
    NguoiDung(const NguoiDung&) = delete;
    NguoiDung& operator=(const NguoiDung&) = delete;
    NguoiDung(NguoiDung&&) noexcept = default;
    NguoiDung& operator=(NguoiDung&&) noexcept = default;

    [[nodiscard]] common::UserId id() const noexcept { return id_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const Email &email() const noexcept { return email_; }
    [[nodiscard]] const PasswordHash& passwordHash() const noexcept { return passwordHash_; }
    [[nodiscard]] ViTienDauGia& wallet() noexcept { return wallet_; }
    [[nodiscard]] const ViTienDauGia& wallet() const noexcept { return wallet_; }
    void addAuction(common::AuctionId id) { postedAuctions_.push_back(id); }
    void removeAuction(common::AuctionId id);
    [[nodiscard]] std::span<const common::AuctionId> createdAuctions() const noexcept {
        return postedAuctions_;
    }
    void follow(common::AuctionId auctionId);
    void unfollow(common::AuctionId auctionId);
    void removeAuctionEvents(common::AuctionId auctionId);
    [[nodiscard]] bool isFollowing(common::AuctionId auctionId) const noexcept;
    [[nodiscard]] std::span<const common::AuctionId> followedAuctions() const noexcept {
        return followedAuctions_;
    }

    void onAuctionEvent(const AuctionEvent& event) override;

    [[nodiscard]] const std::deque<AuctionEvent>& inbox() const noexcept {
        return inbox_;
    }
    void clearInbox() noexcept {
        inbox_.clear();
    }

private:
    friend class EntityCheckpoint;
    friend class application::AccountService;
    void replacePasswordHash(PasswordHash value) { passwordHash_=std::move(value); }
    common::UserId id_;
    std::string name_;
    PasswordHash passwordHash_;
    Email email_;
    ViTienDauGia wallet_{Money::zero()};
    std::vector<common::AuctionId> postedAuctions_;
    std::vector<common::AuctionId> followedAuctions_;
    std::deque<AuctionEvent> inbox_;
};

} // namespace auction::domain
