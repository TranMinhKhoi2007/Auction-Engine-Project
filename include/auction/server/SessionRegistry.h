#pragma once
#include "auction/application/SessionService.h"
#include "auction/server/ClientConnection.h"
#include "auction/domain/ports/ILogger.h"
#include <QHash>
namespace auction::server {
class SessionRegistry final {
public:
    SessionRegistry(application::SessionService &,const domain::IClock &,domain::ILogger &security);
    void bind(ClientConnection &,const application::SessionView &);
    void detach(ClientConnection &);
    void apply(const std::vector<application::RevokedSession> &);
    void purgeExpired();
    [[nodiscard]] std::optional<AuthContext> context(const ClientConnection &) const;
    [[nodiscard]] static QString reasonName(application::RevocationReason);
private:
    void notify(ClientConnection &,QString reason);
    application::SessionService &sessions_;
    const domain::IClock &clock_;
    domain::ILogger &security_;
    QHash<QString,QPointer<ClientConnection>> connections_;
};
}
