#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/ports/IClock.h"
#include "auction/domain/ports/ILogger.h"
#include <functional>
#include <optional>
#include <queue>
#include <unordered_set>
#include <vector>

namespace auction::infrastructure {

class EventLoop {
public:
    using Callback = std::function<void()>;

    explicit EventLoop(const domain::IClock& clock, domain::ILogger* logger = nullptr);

    common::EventId schedule(common::SteadyTime time, Callback callback);
    common::EventId scheduleAfter(common::Millis delay, Callback callback);
    common::EventId scheduleEvery(common::Millis period, Callback callback);

    void cancel(common::EventId id);

    [[nodiscard]] std::optional<common::SteadyTime> nextEventTime();
    size_t runDue();

    void setOnChanged(std::function<void()> onChanged);

private:
    struct ScheduledTask {
        common::EventId id;
        common::SteadyTime time;
        uint64_t sequence;
        Callback callback;
        common::Millis period{common::Millis(0)};

        bool operator>(const ScheduledTask& other) const {
            if (time != other.time) {
                return time > other.time;
            }
            return sequence > other.sequence;
        }
    };

    void purgeCancelled();
    void notifyChanged();

    const domain::IClock& clock_;
    domain::ILogger* logger_{nullptr};
    std::function<void()> onChanged_;

    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, std::greater<ScheduledTask>> tasks_;
    std::unordered_set<common::EventId> cancelled_;
    uint64_t nextId_{1};
    uint64_t sequenceCounter_{0};
    bool isRunningDue_{false};
    bool pendingNotification_{false};
};

} // namespace auction::infrastructure
