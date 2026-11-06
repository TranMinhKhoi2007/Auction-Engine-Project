#include "auction/application/SessionService.h"
#include "auction/domain/values/SecretBuffer.h"
#include <algorithm>
#include <stdexcept>
namespace auction::application {
namespace {
bool equalDigest(const domain::ITokenGenerator::Digest &a,const domain::ITokenGenerator::Digest &b) {
    unsigned difference=0;
    for(size_t i=0;i<a.size();++i)difference|=static_cast<unsigned>(a[i]^b[i]);
    // Volatile output prevents replacing the full comparison with an early exit.
    volatile unsigned result=difference;return result==0;
}
}
SessionService::SessionService(const domain::IClock &clock,const domain::ITokenGenerator &generator,SessionConfig config)
    :clock_(clock),generator_(generator),config_(config) {
    if(config_.idleTtl.count()<=0 || config_.absoluteTtl.count()<=0 || config_.maximumPerUser==0)
        throw std::invalid_argument("Invalid session configuration");
}
CreatedSession SessionService::create(common::UserId user,ClientInfo client) {
    auto token=generator_.generate(domain::kSessionTokenBytes);
    domain::SecretBuffer wipe(token);const auto digest=generator_.digest(token);
    for(const auto &[id,s]:sessions_) { (void)id;if(equalDigest(s.tokenHash,digest))throw std::runtime_error("Token generator collision"); }
    CreatedSession result;
    std::vector<SessionView> previous;
    for(const auto &[id,s]:sessions_) {(void)id;if(s.view.userId==user)previous.push_back(s.view);}
    std::sort(previous.begin(),previous.end(),[](const auto &a,const auto &b){return a.createdAt==b.createdAt?std::stoull(a.sessionId)<std::stoull(b.sessionId):a.createdAt<b.createdAt;});
    while(previous.size()>=config_.maximumPerUser) {
        const auto removed=revoke(previous.front().sessionId,RevocationReason::Evicted);
        result.evicted.insert(result.evicted.end(),removed.begin(),removed.end());previous.erase(previous.begin());
    }
    const auto now=clock_.now();
    const auto absolute=now+config_.absoluteTtl;
    SessionView view{std::to_string(nextId_++),user,now,std::min(now+config_.idleTtl,absolute),absolute,std::move(client)};
    sessions_.emplace(view.sessionId,StoredSession{view,digest});
    result.session=view;result.expiresAt=expiresAt(view.sessionId);result.token=token;return result;
}
std::optional<SessionView> SessionService::validate(std::string_view token) {
    const auto digest=generator_.digest(token);StoredSession *match=nullptr;
    for(auto &[id,s]:sessions_) {(void)id;if(equalDigest(s.tokenHash,digest))match=&s;}
    if(!match || !touch(match->view.sessionId))return std::nullopt;
    return match->view;
}
bool SessionService::isValid(std::string_view id) const {
    const auto it=sessions_.find(id);
    return it!=sessions_.end() && clock_.now()<it->second.view.idleExpiresAt && clock_.now()<it->second.view.absoluteExpiresAt;
}
bool SessionService::touch(std::string_view id) {
    if(!isValid(id))return false;
    auto &s=sessions_.find(id)->second.view;s.idleExpiresAt=std::min(clock_.now()+config_.idleTtl,s.absoluteExpiresAt);return true;
}
std::optional<SessionView> SessionService::view(std::string_view id) const {
    const auto it=sessions_.find(id);return it==sessions_.end()?std::nullopt:std::optional<SessionView>{it->second.view};
}
common::WallTime SessionService::expiresAt(std::string_view id) const {
    const auto value=view(id);if(!value)return clock_.wallNow();
    return clock_.wallNow()+std::chrono::duration_cast<common::WallTime::duration>(std::min(value->idleExpiresAt,value->absoluteExpiresAt)-clock_.now());
}
std::vector<RevokedSession> SessionService::revoke(std::string_view id,RevocationReason reason) {
    const auto it=sessions_.find(id);if(it==sessions_.end())return {};
    std::vector<RevokedSession> result{{it->second.view.sessionId,it->second.view.userId,reason}};
    sessions_.erase(it);return result;
}
std::vector<RevokedSession> SessionService::revokeAllForUser(common::UserId user,RevocationReason reason) {
    std::vector<std::string> ids;for(const auto &[id,s]:sessions_)if(s.view.userId==user)ids.push_back(id);
    std::vector<RevokedSession> result;
    for(const auto &id:ids){auto removed=revoke(id,reason);result.insert(result.end(),removed.begin(),removed.end());}return result;
}
std::vector<RevokedSession> SessionService::purgeExpired() {
    std::vector<std::string> ids;for(const auto &[id,s]:sessions_){(void)s;if(!isValid(id))ids.push_back(id);}
    std::vector<RevokedSession> result;
    for(const auto &id:ids){auto removed=revoke(id,RevocationReason::Expired);result.insert(result.end(),removed.begin(),removed.end());}return result;
}
}
