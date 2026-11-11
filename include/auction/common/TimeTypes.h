#pragma once

#include <chrono>

namespace auction::common {

using SteadyTime = std::chrono::steady_clock::time_point;
using WallTime = std::chrono::system_clock::time_point;
using Millis = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;

} // namespace auction::common
