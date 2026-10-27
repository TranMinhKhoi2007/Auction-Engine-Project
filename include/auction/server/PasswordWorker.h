#pragma once
#include "auction/domain/ports/IPasswordHasher.h"
#include "auction/domain/AuctionConfig.h"
#include <QObject>
#include <QThreadPool>
#include <QHash>
#include <functional>
#include <memory>
#include <exception>
#include <optional>
namespace auction::server {
struct PasswordResult {
    bool verified{};
    std::optional<domain::PasswordHash> hash;
    std::exception_ptr error;
};
using PasswordResultPtr=std::shared_ptr<PasswordResult>;
class PasswordWorker final : public QObject {
    Q_OBJECT
public:
    using Callback=std::function<void(PasswordResult)>;
    explicit PasswordWorker(std::shared_ptr<const domain::IPasswordHasher>,QObject *parent=nullptr,
                            int threads=domain::kPasswordWorkerThreads,int queueLimit=domain::kPasswordWorkerQueueLimit);
    ~PasswordWorker() override;
    [[nodiscard]] bool hash(std::string_view,Callback);
    [[nodiscard]] bool verify(std::string_view,domain::PasswordHash,bool rehash,Callback);
    void shutdown();
    void restart();
    [[nodiscard]] qsizetype outstanding() const noexcept {return callbacks_.size();}
signals:
    void completed(quint64 id,PasswordResultPtr result);
private:
    bool submit(std::string_view,std::optional<domain::PasswordHash>,bool,Callback);
    std::shared_ptr<const domain::IPasswordHasher> hasher_;
    QThreadPool pool_;
    QHash<quint64,Callback> callbacks_;
    quint64 nextId_{};
    int maximumOutstanding_{};
    bool stopping_{};
};
}
Q_DECLARE_METATYPE(auction::server::PasswordResultPtr)
