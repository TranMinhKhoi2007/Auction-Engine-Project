#pragma once
#include <array>
#include <string_view>
namespace auction::protocol {
inline constexpr std::array<std::string_view, 20> kMethods = {
    "sys.hello", "sys.ping", "auth.resume", "auth.register", "auth.login",
    "auth.logout", "wallet.get", "wallet.topUp", "inbox.get", "auction.list",
    "auction.get", "auction.create", "auction.approve", "auction.cancel",
    "auction.closeEarly", "auction.follow", "auction.unfollow", "auction.join",
    "auction.placeBid", "auth.getUser"
};
}
