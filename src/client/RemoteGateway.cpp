#include "auction/client/RemoteGateway.h"
#include "auction/protocol/Envelope.h"
#include "auction/protocol/JsonCodec.h"
#include "auction/domain/values/SecretBuffer.h"
#include "auction/transport/TlsConfiguration.h"
#include <QSslError>
namespace auction::client {
RemoteGateway::RemoteGateway(ServerSyncedClock &clock,QObject *parent,int timeoutMs)
    : IAuctionGateway(parent),clock_(clock),timeoutMs_(timeoutMs) {
    lastSendTick_=clock_.now();
    // Preserve room for existing auction details with long bid histories;
    // the server already refuses responses larger than its outgoing queue.
    socket_.setMaxAllowedIncomingMessageSize(protocol::kMaxQueuedBytes);
    socket_.setMaxAllowedIncomingFrameSize(protocol::kMaxQueuedBytes);
    reconnect_.setSingleShot(true);
    connect(&reconnect_,&QTimer::timeout,this,[this] {setState(ConnectionState::Reconnecting);openSocket();});
    connect(&socket_,&QWebSocket::sslErrors,this,[this](const QList<QSslError> &errors) {
        QString message="Không xác minh được chứng chỉ máy chủ. Hãy chọn chứng chỉ LAN từ chủ server.";
        for(const auto &error:errors) {
            if(error.error()==QSslError::HostNameMismatch) {message="Chứng chỉ không khớp IP/tên máy chủ. Kiểm tra địa chỉ hoặc nhờ chủ server cấp lại chứng chỉ.";break;}
            if(error.error()==QSslError::CertificateExpired)message="Chứng chỉ máy chủ đã hết hạn. Hãy liên hệ chủ server.";
        }
        tlsProblem(message); // Do not ignore SSL errors; Qt rejects the handshake.
    });
    connect(&socket_,&QWebSocket::connected,this,[this] {
        request("sys.hello",{{"protocolVersion",protocol::kVersion},{"clientName","AuctionApp"}},this,[this](auto r) {
            if(!r) {emit clientLog("ERROR",r.error->message);socket_.close();return;}
            try {
                const auto hello=protocol::readObject(*r.value);
                if(protocol::readInteger(hello.value("protocolVersion"))!=protocol::kVersion)throw protocol::ProtocolError("Phiên bản không tương thích","ProtocolMismatch");
                protocol::readString(hello.value("serverVersion"));protocol::readInteger(hello.value("serverNowMs"));
            } catch(const protocol::ProtocolError &e) {emit clientLog("ERROR",QString::fromUtf8(e.what()));socket_.close();return;}
            if(sessionToken_.empty()) {finishHandshake();return;}
            request("auth.resume",{{"sessionToken",QString::fromStdString(sessionToken_)}},this,[this](auto resumed) {
                if(!resumed && resumed.error->type=="ConnectionLost")return;
                finishHandshake();
                if(!resumed) {endSession("Expired");return;}
                try {emit sessionResumed(protocol::fromJson<application::AccountDto>(protocol::readObject(*resumed.value).value("account")));emit refreshRequested();}
                catch(const std::exception &) {endSession("Expired");socket_.close();}
            });
        });
    });
    connect(&socket_,&QWebSocket::textMessageReceived,this,&RemoteGateway::receive);
    connect(&socket_,&QWebSocket::disconnected,this,&RemoteGateway::disconnected);
    connect(&socket_,&QWebSocket::errorOccurred,this,[this](QAbstractSocket::SocketError) {
        emit clientLog("WARN","Không thể kết nối tới máy chủ");
        if(socket_.state()==QAbstractSocket::UnconnectedState) disconnected();
    });
    timeouts_.setInterval(protocol::kDeadlineScanIntervalMs);
    connect(&timeouts_,&QTimer::timeout,this,[this] {
        flushRequests();
        const auto keys=pending_.keys();
        for(auto id:keys) {
            if(!pending_.contains(id) || clock_.now()<pending_[id].deadline) continue;
            auto p=pending_.take(id);
            p.message.fill(QChar(0));p.message.clear();
            if(p.context) p.callback({std::nullopt,GatewayError{"Timeout",{},"Không rõ kết quả, hãy kiểm tra lại",{}},0,{}});
            if(p.method!="sys.hello" && p.method!="sys.ping") emit refreshRequested();
            else socket_.close();
        }
    });timeouts_.start();
    sync_.setInterval(protocol::kClockSyncIntervalMs);connect(&sync_,&QTimer::timeout,this,&RemoteGateway::syncClock);
}
RemoteGateway::~RemoteGateway() {clearToken();socket_.disconnect(this);reconnect_.stop();sync_.stop();timeouts_.stop();socket_.abort();}
void RemoteGateway::setState(ConnectionState state) {if(state_==state)return;state_=state;emit connectionStateChanged(state);}
void RemoteGateway::connectToServer(QUrl url) {
    switching_=true;reconnect_.stop();sync_.stop();socket_.abort();failPending();switching_=false;
    url_=std::move(url);clock_.reset();lastSequence_=0;backoff_=0;
    tlsBlocked_=false;endSession("ServerChanged");setState(ConnectionState::Connecting);openSocket();
}
void RemoteGateway::retryNow() {
    reconnect_.stop();if(socket_.state()==QAbstractSocket::ConnectedState)return;
    switching_=true;socket_.abort();switching_=false;
    tlsBlocked_=false;setState(ConnectionState::Reconnecting);openSocket();
}
void RemoteGateway::tlsProblem(const QString &message) {
    tlsBlocked_=true;reconnect_.stop();emit clientLog("ERROR",message);emit connectionProblem(message);
    setState(ConnectionState::Disconnected);
}
void RemoteGateway::openSocket() {
    if(!transport::validServerUrl(url_)) {tlsProblem("Địa chỉ cần dạng wss://IP:port. ws:// chỉ dùng trên chính máy này.");return;}
    try {
        if(url_.scheme()=="wss")socket_.setSslConfiguration(transport::clientTls(caFile_));
        socket_.open(url_);
    } catch(const std::exception &e) {tlsProblem(QString::fromUtf8(e.what()));}
}
void RemoteGateway::request(QString method,QJsonObject params,QObject *context,Callback<QJsonValue> callback) {
    QPointer<QObject> guard(context?context:this);
    if(socket_.state()!=QAbstractSocket::ConnectedState || (state_!=ConnectionState::Connected && method!="sys.hello" && method!="auth.resume")) {
        QTimer::singleShot(0,guard,[guard,callback=std::move(callback)] {if(guard) callback({std::nullopt,GatewayError{"ConnectionLost",{},"Mất kết nối tới máy chủ",{}},0,{}});});return;
    }
    const auto id=++nextId_;
    const int deadlineMs=timeoutMs_==protocol::kRequestTimeoutMs && (method=="auth.register" || method=="auth.login")?protocol::kAuthenticationRequestTimeoutMs:timeoutMs_;
    pending_.insert(id,{guard,std::move(callback),clock_.now(),clock_.localWallNow(),method,protocol::encode(protocol::request(id,method,params)),false,clock_.now()+common::Millis(deadlineMs)});
    flushRequests();
}
void RemoteGateway::flushRequests() {
    const auto now=clock_.now();
    sendTokens_=std::min(double(protocol::kClientRateBurst),sendTokens_+std::chrono::duration<double>(now-lastSendTick_).count()*protocol::kClientRatePerSecond);
    lastSendTick_=now;
    if(socket_.state()!=QAbstractSocket::ConnectedState)return;
    const auto keys=pending_.keys();
    for(auto id:keys) {
        auto &p=pending_[id];if(p.sentOnWire)continue;
        if(now>=p.deadline)continue;
        if(!p.context){p.message.fill(QChar(0));p.message.clear();pending_.remove(id);continue;}
        if(sendTokens_<1.0)break;
        sendTokens_-=1.0;p.sent=clock_.now();p.sentWall=clock_.localWallNow();p.sentOnWire=true;
        auto message=std::move(p.message);p.message.clear();socket_.sendTextMessage(message);message.fill(QChar(0));message.clear();
    }
}
void RemoteGateway::failPending() {
    auto pending=std::move(pending_);pending_.clear();
    for(auto &p:pending) {p.message.fill(QChar(0));p.message.clear();if(p.context)p.callback({std::nullopt,GatewayError{"ConnectionLost",{},"Mất kết nối tới máy chủ",{}},0,{}});}
}
void RemoteGateway::disconnected() {
    sync_.stop();setState(ConnectionState::Disconnected);failPending();
    if(switching_ || tlsBlocked_) return;
    if(static_cast<int>(socket_.closeCode())==protocol::kSessionReplacedCloseCode && !sessionToken_.empty())endSession("Replaced");
    if(!url_.isEmpty() && !reconnect_.isActive()) {
        reconnect_.start(protocol::kReconnectMs[std::min(backoff_,2)]);backoff_=std::min(backoff_+1,2);
    }
}
void RemoteGateway::receive(const QString &message) {
    try {
        QJsonParseError parseError;const auto doc=QJsonDocument::fromJson(message.toUtf8(),&parseError);
        if(parseError.error!=QJsonParseError::NoError || !doc.isObject()) throw protocol::ProtocolError("JSON phản hồi không hợp lệ");
        const auto o=doc.object();if(protocol::readInteger(o.value("v"))!=protocol::kVersion) throw protocol::ProtocolError("Sai phiên bản","ProtocolMismatch");
        const auto now=protocol::readInteger(o.value("serverNowMs"));
        if(o.contains("event")) {
            const auto event=protocol::readString(o.value("event"));
            if(event=="session.revoked") {endSession(protocol::readString(protocol::readObject(o.value("payload")).value("reason")));return;}
            if(event!="event.auction") return;
            const auto sequence=protocol::readId(o.value("seq"));
            if(lastSequence_ && sequence!=lastSequence_+1) emit clientLog("WARN","Khoảng trống trong sequence event: "+QString::number(lastSequence_)+" → "+QString::number(sequence));
            lastSequence_=sequence;emit eventReceived(protocol::fromJson<domain::AuctionEvent>(o.value("payload")));return;
        }
        const auto id=protocol::readId(o.value("id"));if(!pending_.contains(id))return;
        auto p=pending_.value(id);GatewayResult<QJsonValue> result;result.serverNowMs=now;
        if(!o.value("ok").isBool()) throw protocol::ProtocolError("Thiếu trạng thái phản hồi");
        if(o.value("ok").toBool()) {
            if(!o.contains("result")) throw protocol::ProtocolError("Thiếu kết quả");
            auto payload=o.value("result");
            if(p.method=="auth.register" || p.method=="auth.login") {
                auto auth=protocol::readObject(payload);
                // Keep the bearer secret inside the gateway; callbacks and UI metadata never receive it.
                protocol::fromJson<application::AccountDto>(auth.value("account"));
                protocol::readInteger(auth.value("expiresAtMs"));
                auto token=protocol::readString(auth.take("sessionToken")).toStdString();
                if(token.size()!=43)throw protocol::ProtocolError("Phiên không hợp lệ");
                clearToken();sessionToken_=std::move(token);payload=auth;
            }
            if(p.method=="auth.logout")clearToken();
            result.value=payload;result.metadata=payload.toObject();
            if(p.method=="sys.hello" || p.method=="sys.ping") {
                clock_.sample(now,p.sentWall,clock_.localWallNow(),std::chrono::duration_cast<common::Millis>(clock_.now()-p.sent));
                if(result.metadata.contains("accountCount"))emit accountCountChanged(protocol::readId(result.metadata.value("accountCount")));
                emit clockSynchronized();
            }
        } else {
            result.error=protocol::fromJson<GatewayError>(o.value("error"));
            if(result.error->type=="Forbidden")emit forbidden();
            if(result.error->type=="Unauthorized" && p.method!="auth.resume" && !sessionToken_.empty())endSession("Expired");
        }
        pending_.remove(id);
        if(p.context) p.callback(std::move(result));
    } catch(const std::exception &) {emit clientLog("WARN","Nhận phản hồi không hợp lệ");socket_.close(QWebSocketProtocol::CloseCodeProtocolError,"Invalid response");}
}
void RemoteGateway::clearToken() {domain::SecretBuffer wipe(sessionToken_);}
void RemoteGateway::endSession(const QString &reason) {clearToken();emit sessionEnded(reason);}
void RemoteGateway::finishHandshake() {backoff_=0;reconnect_.stop();setState(ConnectionState::Connected);sync_.start();}
void RemoteGateway::syncClock() {request("sys.ping",{},this,[](auto){});}
}
