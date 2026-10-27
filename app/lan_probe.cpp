#include "auction/transport/TlsConfiguration.h"
#include "auction/protocol/Envelope.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QWebSocket>
#include <QTimer>
#include <iostream>
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);QCommandLineParser parser;parser.addHelpOption();
    parser.addOption({"server","WSS URL","url"});parser.addOption({"ca-cert","Public LAN CA PEM","file"});parser.process(app);
    const QUrl url(parser.value("server"));
    if(url.scheme()!="wss" || !auction::transport::validServerUrl(url)) {std::cerr<<"Pass a valid --server wss://host:port\n";return 1;}
    QWebSocket socket;
    try {socket.setSslConfiguration(auction::transport::clientTls(parser.value("ca-cert")));}
    catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
    QObject::connect(&socket,&QWebSocket::connected,&app,[&]{socket.sendTextMessage(auction::protocol::encode(auction::protocol::request(1,"sys.hello",{{"protocolVersion",auction::protocol::kVersion},{"clientName","TLS LAN probe"}})));});
    QObject::connect(&socket,&QWebSocket::sslErrors,&app,[&](const QList<QSslError> &){std::cerr<<"Certificate verification failed\n";});
    QObject::connect(&socket,&QWebSocket::errorOccurred,&app,[&](QAbstractSocket::SocketError){std::cerr<<"Connection failed: "<<socket.errorString().toStdString()<<'\n';app.exit(1);});
    QObject::connect(&socket,&QWebSocket::textMessageReceived,&app,[&](const QString &text){
        const auto result=QJsonDocument::fromJson(text.toUtf8()).object();
        if(result.value("id").toInt()!=1)return;
        if(!result.value("ok").toBool() || result.value("result").toObject().value("protocolVersion").toInt()!=auction::protocol::kVersion) {app.exit(1);return;}
        std::cout<<"TLS certificate verified; WebSocket hello OK: "<<url.toString().toStdString()<<std::endl;socket.close();app.quit();
    });
    QTimer::singleShot(10000,&app,[&]{std::cerr<<"Connection timeout\n";app.exit(2);});socket.open(url);return app.exec();
}
