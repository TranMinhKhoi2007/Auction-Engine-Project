#pragma once
#include "auction/common/StrongId.h"
#include "auction/server/AuthContext.h"
#include "auction/server/RateLimiter.h"
#include "auction/server/ServerConfig.h"
#include <QWebSocket>
#include <QTcpSocket>
#include <QPointer>
#include <deque>
#include <optional>
namespace auction::server {
class ClientConnection final : public QObject {
public:
    ClientConnection(QWebSocket *socket,QTcpSocket *transport,const ServerConfig &config,QObject *parent);
    void send(const QString &text);
    void closeReplaced();
    QPointer<QWebSocket> socket;
    std::optional<AuthContext> auth;
    QString clientName,lastSessionReason;
    bool authPending{};
    bool closing{};
    quint64 authEpoch{};
    bool hello{};
    RateLimiter limiter;
    QElapsedTimer age;
    qint64 lastPong{};
    qint64 lastPing{};
    int rateViolations{};
    QString address() const;
private:
    QString address_;
    QPointer<QTcpSocket> transport_;
};
}
