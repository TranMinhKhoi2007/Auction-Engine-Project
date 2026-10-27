#pragma once
#include "auction/protocol/ProtocolConfig.h"
#include "auction/application/SessionService.h"
#include "auction/application/LoginThrottle.h"
#include <QHostAddress>
namespace auction::server {
struct ServerConfig {
    // Empty only for isolated in-memory fixtures. The executable supplies a file path.
    std::string databasePath;
    QString tlsCertificateFile;
    QString tlsKeyFile;
    QHostAddress host{QHostAddress::LocalHost};
    quint16 port{8765};
    int maxConnections{protocol::kMaxConnections};
    int helloTimeoutMs{protocol::kUnauthIdleTimeoutMs};
    int heartbeatIntervalMs{protocol::kHeartbeatIntervalMs};
    int heartbeatTimeoutMs{protocol::kHeartbeatTimeoutMs};
    int ratePerSecond{protocol::kRatePerSecond};
    int rateBurst{protocol::kRateBurst};
    uint32_t passwordHashIterations{domain::kPasswordHashIterations};
    application::SessionConfig sessions;
    application::ThrottleConfig throttle;
    int passwordWorkerThreads{domain::kPasswordWorkerThreads};
    int passwordWorkerQueueLimit{domain::kPasswordWorkerQueueLimit};
};
}
