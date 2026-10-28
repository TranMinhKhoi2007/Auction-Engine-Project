#pragma once
#include "auction/common/StrongId.h"
#include <string>
namespace auction::server {
struct AuthContext {common::UserId userId;std::string sessionId;};
}
