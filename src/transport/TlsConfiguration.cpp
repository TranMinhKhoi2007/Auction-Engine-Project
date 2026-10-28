#include "auction/transport/TlsConfiguration.h"
#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QDateTime>
#include <QHostAddress>
#include <stdexcept>
namespace auction::transport {
namespace {
QByteArray read(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("Không mở được file chứng chỉ/khóa TLS");
    if(file.size()>1024*1024)throw std::runtime_error("File chứng chỉ/khóa TLS quá lớn");
    return file.readAll();
}
QList<QSslCertificate> certificates(const QString &path) {
    const auto certs=QSslCertificate::fromData(read(path));
    if(certs.isEmpty())throw std::runtime_error("File chứng chỉ TLS không hợp lệ (cần PEM)");
    const auto now=QDateTime::currentDateTimeUtc();
    for(const auto &cert:certs)if(cert.isNull() || cert.effectiveDate()>now || cert.expiryDate()<=now)
        throw std::runtime_error("Chứng chỉ TLS đã hết hạn hoặc chưa có hiệu lực");
    return certs;
}
void supported() {if(!QSslSocket::supportsSsl())throw std::runtime_error("Qt chưa có TLS backend hoạt động");}
}
QSslConfiguration serverTls(const QString &certificateFile,const QString &keyFile) {
    supported();auto config=QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2OrLater);
    config.setPeerVerifyMode(QSslSocket::VerifyNone); // Server authentication; accounts authenticate clients.
    config.setLocalCertificateChain(certificates(certificateFile));
    auto pem=read(keyFile);QSslKey key(pem,QSsl::Rsa);
    if(key.isNull())key=QSslKey(pem,QSsl::Ec);
    pem.fill('\0');
    if(key.isNull())throw std::runtime_error("Khóa riêng TLS không hợp lệ (cần RSA/EC PEM không mã hóa)");
    config.setPrivateKey(key);return config;
}
QSslConfiguration clientTls(const QString &caFile) {
    supported();auto config=QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2OrLater);config.setPeerVerifyMode(QSslSocket::VerifyPeer);
    if(!caFile.isEmpty())config.addCaCertificates(certificates(caFile));
    return config;
}
bool validServerUrl(const QUrl &url) {
    if(!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment() ||
       (!url.path().isEmpty() && url.path()!="/") || url.port(1)<=0 || url.port(1)>65535)return false;
    if(url.scheme()=="wss")return true;
    // Plain WebSocket is retained only for local development and legacy fixtures.
    return url.scheme()=="ws" && (url.host().compare("localhost",Qt::CaseInsensitive)==0 || QHostAddress(url.host()).isLoopback());
}
}
