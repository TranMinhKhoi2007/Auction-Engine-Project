#include "auction/server/SessionRegistry.h"
#include "auction/protocol/Envelope.h"
namespace auction::server {
SessionRegistry::SessionRegistry(application::SessionService &sessions,const domain::IClock &clock,domain::ILogger &security)
    :sessions_(sessions),clock_(clock),security_(security) {}
QString SessionRegistry::reasonName(application::RevocationReason reason) {
    switch(reason) {
    case application::RevocationReason::LoggedOut:return "LoggedOut";
    case application::RevocationReason::Expired:return "Expired";
    case application::RevocationReason::Replaced:return "Replaced";
    case application::RevocationReason::Evicted:return "Evicted";
    }
    return "Expired";
}
void SessionRegistry::notify(ClientConnection &c,QString reason) {
    c.lastSessionReason=reason;
    const auto now=std::chrono::duration_cast<common::Millis>(clock_.wallNow().time_since_epoch()).count();
    c.send(protocol::encode({{"v",protocol::kVersion},{"event","session.revoked"},{"serverNowMs",now},{"payload",QJsonObject{{"reason",reason}}}}));
}
void SessionRegistry::bind(ClientConnection &c,const application::SessionView &view) {
    if(c.auth && c.auth->sessionId!=view.sessionId)
        apply(sessions_.revoke(c.auth->sessionId,application::RevocationReason::LoggedOut));
    detach(c);
    const auto key=QString::fromStdString(view.sessionId);
    if(auto old=connections_.take(key);old && old!=&c) {
        // Revoke the old binding, not the resumed session/token on the new socket.
        old->auth.reset();++old->authEpoch;old->authPending=false;
        notify(*old,"Replaced");
        security_.info("SESSION_REVOKED reason=Replaced user="+view.userId.toString());
        old->closeReplaced();
    }
    c.auth=AuthContext{view.userId,view.sessionId};c.lastSessionReason.clear();connections_.insert(key,&c);
}
void SessionRegistry::detach(ClientConnection &c) {
    if(c.auth) {
        const auto key=QString::fromStdString(c.auth->sessionId);
        if(connections_.value(key)==&c)connections_.remove(key);
        c.auth.reset();
    }
}
void SessionRegistry::apply(const std::vector<application::RevokedSession> &revoked) {
    for(const auto &entry:revoked) {
        const auto reason=reasonName(entry.reason);
        if(auto c=connections_.take(QString::fromStdString(entry.sessionId))) {
            c->auth.reset();notify(*c,reason);
            if(entry.reason==application::RevocationReason::Replaced)c->closeReplaced();
        }
        security_.info("SESSION_REVOKED reason="+reason.toStdString()+" user="+entry.userId.toString());
        if(entry.reason==application::RevocationReason::Expired)security_.info("SESSION_EXPIRED user="+entry.userId.toString());
    }
}
void SessionRegistry::purgeExpired(){apply(sessions_.purgeExpired());}
std::optional<AuthContext> SessionRegistry::context(const ClientConnection &c) const {
    return c.auth && sessions_.isValid(c.auth->sessionId)?c.auth:std::nullopt;
}
}
