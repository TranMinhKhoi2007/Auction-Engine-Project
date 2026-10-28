#pragma once
#include "auction/server/RpcDispatcher.h"
#include "auction/server/EventBroadcaster.h"
#include "auction/server/SessionRegistry.h"
#include "auction/server/PasswordWorker.h"
#include <QWebSocketServer>
#include <QTimer>
#include <QTcpServer>
#include <QHash>
#include <vector>
namespace auction::server {
class AuctionServer final : public QObject {
    Q_OBJECT
public:
    AuctionServer(ServerRuntime &runtime,QObject *parent=nullptr);
    ~AuctionServer() override;
    bool start(ServerConfig config={});
    void stop();
    quint16 port() const { return transport_?transport_->serverPort():0; }
    bool tlsEnabled() const {return !config_.tlsCertificateFile.isEmpty();}
    int connectionCount() const { return static_cast<int>(connections_.size()); }
    SessionRegistry &sessionRegistry() {return registry_;}
    [[nodiscard]] std::vector<std::string> registeredMethods() const {return dispatcher_.registeredMethods();}
    [[nodiscard]] qsizetype passwordTasks() const {return passwords_.outstanding();}
private:
    void accept();
    void receive(ClientConnection &connection,const QString &message);
    void tick();
    bool trackTransport(QTcpSocket *socket);
    ServerRuntime &runtime_;
    application::AuctionEngine &engine_;
    domain::ILogger &logger_;
    SessionRegistry registry_;
    PasswordWorker passwords_;
    RpcDispatcher dispatcher_;
    std::unique_ptr<QWebSocketServer> listener_;
    std::unique_ptr<QTcpServer> transport_;
    QHash<QString,QPointer<QTcpSocket>> transports_;
    QTimer timer_;
    ServerConfig config_;
    std::vector<ClientConnection *> connections_;
    std::shared_ptr<EventBroadcaster> broadcaster_;
    std::optional<common::EventId> purgeEvent_;
};
}
