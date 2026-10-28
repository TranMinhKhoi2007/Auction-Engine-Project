#include "auction/client/RemoteGateway.h"
#include "auction/client/SystemClock.h"
#include "auction/presentation/MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QStyleFactory>
#include <QFileInfo>
#include "auction/transport/TlsConfiguration.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);app.setStyle(QStyleFactory::create("Fusion"));
    QCoreApplication::setApplicationName("AuctionApp");
    QCommandLineParser parser;parser.addHelpOption();parser.addOption({"server","Máy chủ WebSocket","url"});
    parser.addOption({"ca-cert","Chứng chỉ CA của mạng LAN (PEM)","file"});parser.process(app);
    QSettings settings("AuctionEngine","Client");
    const QUrl url(parser.isSet("server")?parser.value("server"):settings.value("serverUrl","ws://127.0.0.1:8765").toString());
    if(!auction::transport::validServerUrl(url))return 1;
    settings.setValue("serverUrl",url.toString());
    auction::client::SystemClock local;auction::client::ServerSyncedClock clock(local);
    auction::client::RemoteGateway gateway(clock);
    const auto bundledCa=QCoreApplication::applicationDirPath()+"/client-trust/lan-ca.pem";
    const auto ca=parser.isSet("ca-cert")?parser.value("ca-cert"):settings.value("caCertificateFile",QFileInfo::exists(bundledCa)?bundledCa:QString()).toString();
    gateway.setCaCertificateFile(ca);settings.setValue("caCertificateFile",ca);
    auto bridge=std::make_shared<auction::presentation::QtEventBridge>(gateway);
    auto log=std::make_shared<auction::presentation::QtLogSink>();
    auction::presentation::MainWindow window(gateway,bridge,log);window.show();gateway.connectToServer(url);
    return app.exec();
}
