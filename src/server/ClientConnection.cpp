#include "auction/server/ClientConnection.h"
#include <QTimer>
#include <QMetaMethod>
namespace auction::server {
ClientConnection::ClientConnection(QWebSocket *ws,QTcpSocket *transport,const ServerConfig &config,QObject *parent)
    : QObject(parent),socket(ws),limiter(config.ratePerSecond,config.rateBurst),address_(ws->peerAddress().toString()),transport_(transport) {
    ws->setParent(this); transport->setParent(this); age.start();
    transport->setSocketOption(QAbstractSocket::LowDelayOption,1);
    ws->setMaxAllowedIncomingMessageSize(protocol::kMaxImageMessageBytes);
    ws->setMaxAllowedIncomingFrameSize(protocol::kMaxImageMessageBytes);
    ws->setReadBufferSize(static_cast<qint64>(protocol::kMaxImageMessageBytes));
    connect(ws,&QWebSocket::aboutToClose,this,[ws,transport] {
        // Qt 6.11 keeps decoding unread bytes after rejecting an oversized frame,
        // repeatedly starting close timers. The close frame has already been flushed.
        // Drain rejected bytes before leaving the decoder, then finish the TCP
        // write out of stack so the peer receives the WebSocket close code.
        if(ws->closeCode()>=QWebSocketProtocol::CloseCodeProtocolError) {
            QObject::disconnect(transport,QMetaMethod::fromSignal(&QIODevice::readyRead),nullptr,QMetaMethod{});
            transport->readAll();
            QTimer::singleShot(0,ws,[transport]{transport->disconnectFromHost();});
            QTimer::singleShot(protocol::kRejectedFrameCloseGraceMs,ws,[ws]{
                if(ws->state()!=QAbstractSocket::UnconnectedState) ws->abort();
            });
        }
    });
}
QString ClientConnection::address() const { return address_; }
void ClientConnection::send(const QString &text) {
    if(closing || !socket || socket->state()!=QAbstractSocket::ConnectedState) return;
    if(socket->bytesToWrite()+text.toUtf8().size()>protocol::kMaxQueuedBytes) {
        socket->close(QWebSocketProtocol::CloseCodePolicyViolated,"Slow connection"); return;
    }
    socket->sendTextMessage(text);
}
void ClientConnection::closeReplaced() {
    if(closing || !transport_ || !socket)return;
    closing=true;
    // Qt 6.11's non-fixed CloseCode enum cannot represent private code 4001 in
    // standard C++. Write this single RFC 6455 server control frame through the
    // owned transport; do not cast outside the enum range or suppress warnings.
    QObject::disconnect(transport_,QMetaMethod::fromSignal(&QIODevice::readyRead),nullptr,QMetaMethod{});
    transport_->readAll();
    QByteArray payload;
    payload.append(static_cast<char>((protocol::kSessionReplacedCloseCode>>8)&0xff));
    payload.append(static_cast<char>(protocol::kSessionReplacedCloseCode&0xff));payload.append("Replaced");
    QByteArray frame;frame.append(static_cast<char>(0x88));frame.append(static_cast<char>(payload.size()));frame.append(payload);
    transport_->write(frame);transport_->flush();
    QTimer::singleShot(0,this,[this]{if(transport_)transport_->disconnectFromHost();});
    QTimer::singleShot(protocol::kRejectedFrameCloseGraceMs,this,[this]{if(socket && socket->state()!=QAbstractSocket::UnconnectedState)socket->abort();});
}
}
