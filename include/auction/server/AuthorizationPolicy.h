#pragma once
#include <array>
#include <string_view>
namespace auction::server {
enum class ResourceRule {Public,Self,CreateAsSelf,Owner,ExistingParticipantRules};
struct AuthorizationEntry {std::string_view method;bool requiresSession;ResourceRule resource;};
class AuthorizationPolicy final {
public:
    inline static constexpr std::array<AuthorizationEntry,24> entries{{
        {"sys.hello",false,ResourceRule::Public},{"sys.ping",false,ResourceRule::Public},
        {"auth.register",false,ResourceRule::Public},{"auth.login",false,ResourceRule::Public},
        {"auth.resetPassword",false,ResourceRule::Public},{"auth.changePassword",true,ResourceRule::Self},
        {"auth.resume",false,ResourceRule::Public},{"auth.logout",true,ResourceRule::Self},
        {"auth.getUser",true,ResourceRule::Self},
        {"wallet.get",true,ResourceRule::Self},{"wallet.topUp",true,ResourceRule::Self},
        {"inbox.get",true,ResourceRule::Self},
        {"auction.list",false,ResourceRule::Public},{"auction.get",false,ResourceRule::Public},
        {"auction.image",false,ResourceRule::Public},
        {"auction.create",true,ResourceRule::CreateAsSelf},
        {"auction.approve",true,ResourceRule::Owner},{"auction.cancel",true,ResourceRule::Owner},
        {"auction.delete",true,ResourceRule::Owner},
        {"auction.closeEarly",true,ResourceRule::Owner},
        {"auction.follow",true,ResourceRule::ExistingParticipantRules},{"auction.unfollow",true,ResourceRule::ExistingParticipantRules},
        {"auction.join",true,ResourceRule::ExistingParticipantRules},{"auction.placeBid",true,ResourceRule::ExistingParticipantRules}
    }};
    [[nodiscard]] static constexpr const AuthorizationEntry *find(std::string_view method) noexcept {
        for(const auto &entry:entries)if(entry.method==method)return &entry;
        return nullptr; // Deny by default, independently of registered handlers.
    }
};
}
