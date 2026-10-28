#pragma once

#include "auction/common/TimeTypes.h"
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace auction::domain {

using namespace std::chrono_literals;

inline constexpr auto kMinAuctionDuration = 5s;
inline constexpr auto kMinStartLead = 1s;
inline constexpr int kDefaultDepositPercent = 10;
inline constexpr int64_t kCountdownLastSeconds = 10;
inline constexpr auto kCountdownCoarseInterval = 10s;
inline constexpr size_t kMaxInboxSize = 200;
inline constexpr size_t kMinPasswordLength = 8;
inline constexpr size_t kMaxPasswordLength = 128;
inline constexpr size_t kMaxDisplayNameLength = 64;
inline constexpr size_t kMaxEmailLength = 254;
inline constexpr size_t kMaxEmailLocalLength = 64;
inline constexpr size_t kMaxEncodedPasswordHashBytes = 160;
inline constexpr size_t kMaxHashBase64ComponentBytes = 64;
inline constexpr uint32_t kPasswordHashIterations = 600000;
inline constexpr uint32_t kMaxStoredPasswordHashIterations = 10000000;
inline constexpr size_t kPasswordSaltBytes = 16;
inline constexpr size_t kPasswordKeyBytes = 32;
inline constexpr auto kSessionIdleTtl = 2h;
inline constexpr auto kSessionAbsoluteTtl = 24h;
inline constexpr size_t kMaxSessionsPerUser = 5;
inline constexpr size_t kSessionTokenBytes = 32;
inline constexpr auto kSessionPurgeInterval = 60s;
inline constexpr unsigned kLoginEmailFailureLimit = 5;
inline constexpr auto kLoginInitialLock = 30s;
inline constexpr auto kLoginMaximumLock = 15min;
inline constexpr size_t kLoginAddressFailureLimit = 20;
inline constexpr auto kLoginAddressWindow = 10min;
inline constexpr size_t kRegistrationAddressLimit = 5;
inline constexpr auto kRegistrationWindow = 10min;
inline constexpr int kPasswordWorkerThreads = 2;
inline constexpr int kPasswordWorkerQueueLimit = 32;
inline constexpr auto kDriftWarnThreshold = 20ms;
inline constexpr auto kUiRefreshInterval = 100ms;
inline constexpr auto kMaxTimerSleep = 1000ms;

} // namespace auction::domain
