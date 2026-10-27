#include "auction/server/AuctionServer.h"
#include "auction/protocol/JsonCodec.h"
#include "auction/server/ServerRuntime.h"
#include "auction/transport/TlsConfiguration.h"
#include <QSslServer>
#include <algorithm>
namespace auction::server {
AuctionServer::AuctionServer(ServerRuntime &runtime,QObject *parent)
    : QObject(parent),runtime_(runtime),engine_(runtime.engine()),logger_(runtime.logger()),
      registry_(runtime.sessions(),runtime.engine().clock(),runtime.security()),
      passwords_(runtime.passwordHasher(),this,runtime.config().passwordWorkerThreads,runtime.config().passwordWorkerQueueLimit),
      dispatcher_(runtime,registry_,passwords_) {
    connect(&timer_,&QTimer::timeout,this,&AuctionServer::tick);
    broadcaster_=std::make_shared<EventBroadcaster>(engine_.clock(),[this](const QString &text,std::optional<common::UserId> target) {
        const auto snapshot=connections_;
        for(auto *c:snapshot) {
            const auto auth=registry_.context(*c);
            if(c->hello && (!target || (auth && auth->userId==*target)))c->send(text);
        }
    });
    engine_.addGlobalObserver(broadcaster_);
}
AuctionServer::~AuctionServer() {
    stop();
    // Children are destroyed by QObject after our members; detach callbacks first.
    for(auto *c:connections_) if(c->socket) c->socket->disconnect();
    for(auto tcp:transports_) if(tcp) tcp->disconnect(this);
}
bool AuctionServer::start(ServerConfig config) {
    stop();
    config_=std::move(config);
    if(config_.tlsCertificateFile.isEmpty()!=config_.tlsKeyFile.isEmpty())throw std::runtime_error("Cần cấp cả chứng chỉ và khóa riêng TLS");
    if(!tlsEnabled() && !config_.host.isLoopback())throw std::runtime_error("Kết nối LAN cần TLS: hãy cấp --tls-cert và --tls-key");
    listener_=std::make_unique<QWebSocketServer>("AuctionServer",tlsEnabled()?QWebSocketServer::SecureMode:QWebSocketServer::NonSecureMode);
    if(tlsEnabled()) {
        auto ssl=std::make_unique<QSslServer>();
        ssl->setSslConfiguration(transport::serverTls(config_.tlsCertificateFile,config_.tlsKeyFile));
        ssl->setHandshakeTimeout(config_.helloTimeoutMs);
        connect(ssl.get(),&QSslServer::startedEncryptionHandshake,this,[this](QSslSocket *socket){trackTransport(socket);});
        transport_=std::move(ssl);
    } else {
        transport_=std::make_unique<QTcpServer>();
    }
    transport_->setMaxPendingConnections(config_.maxConnections);
    connect(listener_.get(),&QWebSocketServer::newConnection,this,&AuctionServer::accept);
    // QSslServer queues the socket only after TLS succeeds; newConnection can
    // precede that. pendingConnectionAvailable covers both TCP and TLS correctly.
    connect(transport_.get(),&QTcpServer::pendingConnectionAvailable,this,[this] {
        while(transport_->hasPendingConnections()) {
            auto *tcp=transport_->nextPendingConnection();
            if(tcp->state()!=QAbstractSocket::ConnectedState) {tcp->deleteLater();continue;}
            const auto key=tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort());
            if(!transports_.contains(key) && !trackTransport(tcp))continue;
            if(tlsEnabled()) {
                const auto *ssl=qobject_cast<QSslSocket*>(tcp);
                if(!ssl || !ssl->isEncrypted()) {tcp->close();continue;}
            }
            listener_->handleConnection(tcp);
        }
    });
    listener_->setHandshakeTimeout(config_.helloTimeoutMs);
    if(!transport_->listen(config_.host,config_.port)) return false;
    passwords_.restart();
    QPointer<AuctionServer> guard(this);
    purgeEvent_=runtime_.eventLoop().scheduleEvery(domain::kSessionPurgeInterval,[guard]{if(guard)guard->registry_.purgeExpired();});
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.start(std::min(protocol::kServerMaintenanceIntervalMs,std::max(1,config_.helloTimeoutMs)));
    return true;
}
void AuctionServer::stop() {
    timer_.stop();if(transport_)transport_->close();if(listener_)listener_->close();
    if(purgeEvent_){runtime_.eventLoop().cancel(*purgeEvent_);purgeEvent_.reset();}
    const auto snapshot=connections_;
    for(auto *c:snapshot) if(c->socket) c->socket->close(QWebSocketProtocol::CloseCodeGoingAway,"Server shutting down");
    const auto pending=transports_;
    for(auto tcp:pending) if(tcp && tcp->parent()==this) tcp->close();
    passwords_.shutdown();
}
bool AuctionServer::trackTransport(QTcpSocket *tcp) {
    if(transports_.size()>=config_.maxConnections) {tcp->close();tcp->deleteLater();return false;}
    tcp->setParent(this);tcp->setReadBufferSize(static_cast<qint64>(protocol::kMaxImageMessageBytes));
    const auto key=tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort());
    transports_.insert(key,tcp);
    connect(tcp,&QTcpSocket::disconnected,this,[this,key,tcp]{transports_.remove(key);tcp->deleteLater();});
    connect(tcp,&QObject::destroyed,this,[this,key,tcp]{if(transports_.value(key).isNull() || transports_.value(key)==tcp)transports_.remove(key);});
    return true;
}
void AuctionServer::accept() {
    while(listener_->hasPendingConnections()) {
        auto *socket=listener_->nextPendingConnection();
        if(connectionCount()>=config_.maxConnections) { socket->close(QWebSocketProtocol::CloseCodePolicyViolated,"Connection limit"); socket->deleteLater(); continue; }
        const auto key=socket->peerAddress().toString()+":"+QString::number(socket->peerPort());
        auto *tcp=transports_.value(key).data();
        if(!tcp) {socket->close();socket->deleteLater();continue;}
        tcp->disconnect(this); // ownership and cleanup now belong to ClientConnection.
        auto *c=new ClientConnection(socket,tcp,config_,this);
        connections_.push_back(c);
        logger_.info(("CONN_OPEN " + c->address()).toStdString());
        connect(socket,&QWebSocket::textMessageReceived,c,[this,c](const QString &s){receive(*c,s);});
        connect(socket,&QWebSocket::binaryMessageReceived,c,[this,c](const QByteArray &) {
            const auto now=std::chrono::duration_cast<common::Millis>(engine_.clock().wallNow().time_since_epoch()).count();
            c->send(protocol::encode(protocol::failure(0,{"BadRequest",{},"Chỉ nhận tin JSON văn bản",{}},now)));
            c->socket->close(QWebSocketProtocol::CloseCodeDatatypeNotSupported,"Text JSON required");
        });
        connect(socket,&QWebSocket::pong,c,[c](quint64,const QByteArray &){c->lastPong=c->age.elapsed();});
        connect(socket,&QWebSocket::disconnected,this,[this,c,key] {
            registry_.detach(*c);++c->authEpoch;c->authPending=false;
            logger_.info(("CONN_CLOSE "+c->address()).toStdString());
            transports_.remove(key); std::erase(connections_,c); c->deleteLater();
        });
    }
}
void AuctionServer::receive(ClientConnection &c,const QString &message) {
    if(c.closing)return;
    protocol::Request r{};
    const auto now=std::chrono::duration_cast<common::Millis>(engine_.clock().wallNow().time_since_epoch()).count();
    const auto bytes=message.toUtf8();
    if(bytes.size()>static_cast<qsizetype>(protocol::kMaxMessageBytes)) {
        const auto object=QJsonDocument::fromJson(bytes).object();
        if(bytes.size()>static_cast<qsizetype>(protocol::kMaxImageMessageBytes) || object.value("method")!="auction.create" || !object.value("params").toObject().value("imageBase64").isString()) {
            c.socket->close(QWebSocketProtocol::CloseCodeTooMuchData,"Message too large");return;
        }
    }
    try {
        if(!c.limiter.consume()) {
            if(++c.rateViolations>=protocol::kRateViolationsBeforeClose) { c.socket->close(QWebSocketProtocol::CloseCodePolicyViolated,"Rate limit"); return; }
            throw protocol::ProtocolError("Gửi quá nhanh, hãy thử lại sau","RateLimited",{},{{"retryAfterMs",protocol::kRateLimitRetryAfterMs}});
        }
        c.rateViolations=std::max(0,c.rateViolations-1);
        r=protocol::parseRequest(message);
        QPointer<ClientConnection> guard(&c);
        const auto complete=[this,guard,id=r.id](QJsonValue result,std::optional<protocol::ErrorPayload> error) {
            if(!guard || !guard->socket || guard->socket->state()!=QAbstractSocket::ConnectedState)return;
            const auto respondedAt=std::chrono::duration_cast<common::Millis>(engine_.clock().wallNow().time_since_epoch()).count();
            guard->send(protocol::encode(error?protocol::failure(id,*error,respondedAt):protocol::response(id,result,respondedAt)));
        };
        const auto dispatched=dispatcher_.dispatch(c,r,complete);
        if(const auto *result=std::get_if<QJsonValue>(&dispatched))complete(*result,std::nullopt);
    } catch(const std::exception &e) {
        // Recover a valid id without logging the untrusted message (it may contain a password).
        if(r.id==0) {
            try {r.id=protocol::readId(QJsonDocument::fromJson(message.toUtf8()).object().value("id"));}catch(const protocol::ProtocolError &) {}
        }
        const auto error=dispatcher_.mapException(e,r);
        c.send(protocol::encode(protocol::failure(r.id,error,now)));
        if(error.type=="ProtocolMismatch") c.socket->close(QWebSocketProtocol::CloseCodeProtocolError,"Protocol mismatch");
    } catch(...) {
        logger_.error("Internal: unknown exception");
        c.send(protocol::encode(protocol::failure(r.id,{"Internal",{},"Lỗi hệ thống",{}},now)));
    }
}
void AuctionServer::tick() {
    const auto snapshot=connections_;
    for(auto *c:snapshot) {
        if(c->closing)continue;
        if(!c->hello && c->age.elapsed()>=config_.helloTimeoutMs) c->socket->close(QWebSocketProtocol::CloseCodePolicyViolated,"Hello timeout");
        else if(c->age.elapsed()-c->lastPong>=config_.heartbeatTimeoutMs) c->socket->close(QWebSocketProtocol::CloseCodeGoingAway,"Heartbeat timeout");
        else if(c->socket->bytesToWrite()>protocol::kMaxQueuedBytes)c->socket->close(QWebSocketProtocol::CloseCodePolicyViolated,"Slow connection");
        else if(c->age.elapsed()-c->lastPing>=config_.heartbeatIntervalMs){c->lastPing=c->age.elapsed();c->socket->ping();}
    }
}
}
