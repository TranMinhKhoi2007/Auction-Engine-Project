#pragma once
#include "auction/domain/ports/IUnitOfWork.h"
#include "auction/domain/ports/IUserRepository.h"
#include "auction/domain/ports/IAuctionRepository.h"
#include "auction/domain/ports/IClock.h"
#include <functional>
#include <memory>
#include <string>
namespace auction::infrastructure_qt {
// A server-thread identity map and SQL unit of work. Repositories must share it.
class SqliteStore final : public domain::IUnitOfWork {
public:
    SqliteStore(const std::string &path,const domain::IClock &clock,std::function<void()> onFailure={});
    ~SqliteStore() override;
    bool isHealthy() const noexcept override;
    void execute(const domain::MutationContext &,const std::function<void()> &) override;
    std::weak_ptr<domain::IAuctionObserver> afterCommit(std::weak_ptr<domain::IAuctionObserver>) override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend class SqlUserRepository;
    friend class SqlAuctionRepository;
};
class SqlUserRepository final : public domain::IUserRepository {
public:
    explicit SqlUserRepository(std::shared_ptr<SqliteStore> store):store_(std::move(store)) {}
    void save(std::shared_ptr<domain::NguoiDung>) override;
    std::shared_ptr<domain::NguoiDung> findById(common::UserId) const override;
    std::vector<std::shared_ptr<domain::NguoiDung>> findAll() const override;
    std::shared_ptr<domain::NguoiDung> findByEmail(const domain::Email &) const override;
    bool emailExists(const domain::Email &) const override;
private:
    std::shared_ptr<SqliteStore> store_;
};
class SqlAuctionRepository final : public domain::IAuctionRepository {
public:
    explicit SqlAuctionRepository(std::shared_ptr<SqliteStore> store):store_(std::move(store)) {}
    void save(std::shared_ptr<domain::PhienDauGia>) override;
    bool remove(common::AuctionId) override;
    std::shared_ptr<domain::PhienDauGia> findById(common::AuctionId) const override;
    std::vector<std::shared_ptr<domain::PhienDauGia>> findAll() const override;
private:
    std::shared_ptr<SqliteStore> store_;
};
}
