#pragma once
#include <chrono>
#include <cstddef>
namespace auction::protocol {
inline constexpr int kVersion = 2;
inline constexpr std::size_t kMaxMessageBytes = 64 * 1024;
// Only auction.create with an encoded image may exceed the normal request limit.
inline constexpr std::size_t kMaxImageMessageBytes = 384 * 1024;
inline constexpr int kMaxStringBytes = 4 * 1024;
inline constexpr int kMaxConnections = 200;
inline constexpr int kUnauthIdleTimeoutMs = 30000;
inline constexpr int kHeartbeatIntervalMs = 15000;
inline constexpr int kHeartbeatTimeoutMs = 45000;
inline constexpr int kServerMaintenanceIntervalMs = 200;
inline constexpr int kRejectedFrameCloseGraceMs = 250;
inline constexpr int kSessionReplacedCloseCode = 4001;
inline constexpr int kRatePerSecond = 20;
inline constexpr int kRateBurst = 40;
inline constexpr int kRateLimitRetryAfterMs = 1000;
inline constexpr int kRateViolationsBeforeClose = 20;
inline constexpr int kLoginAttempts = 5;
inline constexpr int kLoginWindowMs = 60000;
inline constexpr int kMaxQueuedBytes = 1024 * 1024;
inline constexpr int kRequestTimeoutMs = 10000;
inline constexpr int kAuthenticationRequestTimeoutMs = 30000;
inline constexpr int kDeadlineScanIntervalMs = 50;
inline constexpr int kClientRatePerSecond = 18;
inline constexpr int kClientRateBurst = 10;
inline constexpr int kClockSyncIntervalMs = 30000;
inline constexpr int kClockSampleCount = 5;
inline constexpr int kShutdownPollMs = 200;
inline constexpr int kReconnectMs[] = {1000, 2000, 5000};
}
