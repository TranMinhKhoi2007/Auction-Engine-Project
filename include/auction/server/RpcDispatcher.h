#pragma once
#include "auction/application/AuctionEngine.h"
#include "auction/server/ClientConnection.h"
#include "auction/server/SessionRegistry.h"
#include "auction/server/PasswordWorker.h"
#include "auction/protocol/Envelope.h"
#include <functional>
#include <map>
#include <variant>
namespace auction::server {
class ServerRuntime;
struct Deferred {};
using DispatchResult=std::variant<QJsonValue,Deferred>;
class RpcDispatcher {
public:
    using Completion=std::function<void(QJsonValue,std::optional<protocol::ErrorPayload>)>;
    RpcDispatcher(ServerRuntime &,SessionRegistry &,PasswordWorker &);
    DispatchResult dispatch(ClientConnection &,const protocol::Request &,Completion);
    protocol::ErrorPayload mapException(const std::exception &error,const protocol::Request &request);
    [[nodiscard]] std::vector<std::string> registeredMethods() const;
private:
    using Sync=std::function<QJsonValue(ClientConnection &,const std::optional<AuthContext> &,QJsonObject)>;
    using Async=std::function<void(ClientConnection &,const std::optional<AuthContext> &,QJsonObject,Completion)>;
    void registerAccount(ClientConnection &,QJsonObject,Completion);
    void login(ClientConnection &,QJsonObject,Completion);
    void resetPassword(ClientConnection &,QJsonObject,Completion);
    void changePassword(ClientConnection &,const std::optional<AuthContext> &,QJsonObject,Completion);
    QJsonObject createSession(ClientConnection &,const application::AccountDto &);
    void logForbidden(const ClientConnection &,const protocol::Request &);
    void limited(application::ThrottleResult);
    std::map<QString,std::variant<Sync,Async>> methods_;
    ServerRuntime &runtime_;
    SessionRegistry &registry_;
    PasswordWorker &passwords_;
    application::AuctionEngine &engine_;
    domain::ILogger &logger_;
};
}
