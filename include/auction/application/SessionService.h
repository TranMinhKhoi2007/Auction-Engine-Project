#pragma once
#include "auction/domain/AuctionConfig.h"
#include "auction/domain/ports/IClock.h"
#include "auction/domain/ports/ITokenGenerator.h"
#include "auction/common/StrongId.h"
#include <map>
#include <optional>
#include <vector>
namespace auction::application {
enum class RevocationReason {LoggedOut,Expired,Replaced,Evicted};
struct ClientInfo {std::string clientName,remoteAddress;};
struct SessionConfig {
    common::Millis idleTtl{domain::kSessionIdleTtl},absoluteTtl{domain::kSessionAbsoluteTtl};
    size_t maximumPerUser{domain::kMaxSessionsPerUser};
};
struct SessionView {
    std::string sessionId;
    common::UserId userId;
    common::SteadyTime createdAt,idleExpiresAt,absoluteExpiresAt;
    ClientInfo client;
};
struct RevokedSession {std::string sessionId;common::UserId userId;RevocationReason reason;};
struct CreatedSession {
    std::string token;
    SessionView session;
    common::WallTime expiresAt;
    std::vector<RevokedSession> evicted;
};
class SessionService final {
public:
    SessionService(const domain::IClock &,const domain::ITokenGenerator &,SessionConfig={});
    [[nodiscard]] CreatedSession create(common::UserId,ClientInfo);
    [[nodiscard]] std::optional<SessionView> validate(std::string_view token);
    [[nodiscard]] bool touch(std::string_view sessionId);
    [[nodiscard]] bool isValid(std::string_view sessionId) const;
    [[nodiscard]] std::optional<SessionView> view(std::string_view sessionId) const;
    [[nodiscard]] common::WallTime expiresAt(std::string_view sessionId) const;
    std::vector<RevokedSession> revoke(std::string_view,RevocationReason);
    std::vector<RevokedSession> revokeAllForUser(common::UserId,RevocationReason);
    std::vector<RevokedSession> purgeExpired();
    [[nodiscard]] size_t size() const noexcept {return sessions_.size();}
private:
    struct StoredSession {SessionView view;domain::ITokenGenerator::Digest tokenHash;};
    const domain::IClock &clock_;
    const domain::ITokenGenerator &generator_;
    SessionConfig config_;
    uint64_t nextId_{1};
    std::map<std::string,StoredSession,std::less<>> sessions_;
};
}
