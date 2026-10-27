#include "auction/server/ServerRuntime.h"
#include "auction/server/AuctionServer.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <atomic>
#include <csignal>
#include <iostream>
namespace { std::atomic_bool stopping{}; void signalHandler(int) {stopping.store(true);} }
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv); QCoreApplication::setApplicationName("AuctionServer");
    QCommandLineParser parser; parser.addHelpOption();
    parser.addOption({"host","Bind address","host","127.0.0.1"}); parser.addOption({"port","WebSocket port","port","8765"});
    parser.addOption({"log-dir","Log directory","directory","logs"});
    parser.addOption({"database","SQLite database file","file",QCoreApplication::applicationDirPath()+"/data/auction.sqlite"});
    parser.addOption({"tls-cert","PEM server certificate chain","file"});
    parser.addOption({"tls-key","PEM private key","file"});
    parser.process(app);
    bool valid=false; const auto port=parser.value("port").toUInt(&valid); const QHostAddress host(parser.value("host"));
    if(!valid || port>65535 || host.isNull()) {std::cerr << "Invalid host/port\n";return 1;}
    try {
        auction::server::ServerConfig config; config.host=host; config.port=static_cast<quint16>(port);
        config.tlsCertificateFile=parser.value("tls-cert");config.tlsKeyFile=parser.value("tls-key");
        config.databasePath=parser.value("database").toStdString();
        if(config.databasePath.empty())throw std::invalid_argument("Database path must not be empty");
        auction::server::ServerRuntime runtime(parser.value("log-dir").toStdString(),config);
        auction::server::AuctionServer server(runtime);
        if(!server.start(config)) {std::cerr << "Cannot listen\n";return 1;}
        std::cout << "Server lắng nghe tại " << (server.tlsEnabled()?"wss://":"ws://") << parser.value("host").toStdString() << ':' << server.port() << std::endl;
        std::signal(SIGINT,signalHandler); std::signal(SIGTERM,signalHandler);
        QTimer shutdown; shutdown.setInterval(auction::protocol::kShutdownPollMs);
        QObject::connect(&shutdown,&QTimer::timeout,&app,[&]{if(stopping.load()) app.quit();}); shutdown.start();
        QObject::connect(&app,&QCoreApplication::aboutToQuit,&server,&auction::server::AuctionServer::stop);
        return app.exec();
    } catch(const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
