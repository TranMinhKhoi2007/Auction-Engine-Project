#pragma once
#include "auction/client/IAuctionGateway.h"
#include "auction/client/ServerSyncedClock.h"
#include <QWebSocket>
#include <QTimer>
#include <QPointer>
#include <QMap>
namespace auction::client {
class RemoteGateway final : public IAuctionGateway {
    Q_OBJECT
public:
    explicit RemoteGateway(ServerSyncedClock &clock,QObject *parent=nullptr,int timeoutMs=protocol::kRequestTimeoutMs);
    ~RemoteGateway() override;
    const domain::IClock &clock() const override {return clock_;}
    ConnectionState state() const override {return state_;}
    QUrl serverUrl() const override {return url_;}
    void setCaCertificateFile(QString path) override {caFile_=std::move(path);}
    QString caCertificateFile() const override {return caFile_;}
    void connectToServer(QUrl url) override;
    void retryNow() override;
    void request(QString method,QJsonObject params,QObject *context,Callback<QJsonValue> callback) override;
private:
    struct Pending {
        QPointer<QObject> context;
        Callback<QJsonValue> callback;
        common::SteadyTime sent;
        common::WallTime sentWall;
        QString method;
        QString message;
        bool sentOnWire{};
        common::SteadyTime deadline;
    };
    void setState(ConnectionState state);
    void receive(const QString &message);
    void disconnected();
    void failPending();
    void syncClock();
    void flushRequests();
    void clearToken();
    void endSession(const QString &reason);
    void finishHandshake();
    void openSocket();
    void tlsProblem(const QString &message);
    ServerSyncedClock &clock_;
    QWebSocket socket_;
    QTimer reconnect_,timeouts_,sync_;
    QMap<quint64,Pending> pending_;
    QUrl url_;
    QString caFile_;
    bool tlsBlocked_{};
    ConnectionState state_{ConnectionState::Disconnected};
    quint64 nextId_{},lastSequence_{};
    int backoff_{},timeoutMs_;
    bool switching_{};
    std::string sessionToken_;
    double sendTokens_{protocol::kClientRateBurst};
    common::SteadyTime lastSendTick_{};
};
}
