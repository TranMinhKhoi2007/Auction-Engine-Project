#include "auction/infrastructure/EventLoop.h"
#include "auction/domain/AuctionConfig.h"
#include "auction/domain/exceptions/AuctionException.h"
#include <chrono>

namespace auction::infrastructure {

EventLoop::EventLoop(const domain::IClock& clock, domain::ILogger* logger)
    : clock_(clock), logger_(logger) {}

common::EventId EventLoop::schedule(common::SteadyTime time, Callback callback) {
    common::EventId id{nextId_++};
    tasks_.push(ScheduledTask{
        .id = id,
        .time = time,
        .sequence = ++sequenceCounter_,
        .callback = std::move(callback),
        .period = common::Millis(0)
    });
    notifyChanged();
    return id;
}

common::EventId EventLoop::scheduleAfter(common::Millis delay, Callback callback) {
    return schedule(clock_.now() + delay, std::move(callback));
}

common::EventId EventLoop::scheduleEvery(common::Millis period, Callback callback) {
    if (period <= common::Millis(0)) {
        throw domain::ValidationException("Chu kỳ lặp của sự kiện phải lớn hơn 0");
    }
    common::EventId id{nextId_++};
    tasks_.push(ScheduledTask{
        .id = id,
        .time = clock_.now() + period,
        .sequence = ++sequenceCounter_,
        .callback = std::move(callback),
        .period = period
    });
    notifyChanged();
    return id;
}

void EventLoop::cancel(common::EventId id) {
    cancelled_.insert(id);
    notifyChanged();
}

void EventLoop::purgeCancelled() {
    while (!tasks_.empty()) {
        auto it = cancelled_.find(tasks_.top().id);
        if (it != cancelled_.end()) {
            cancelled_.erase(it);
            tasks_.pop();
        } else {
            break;
        }
    }
}

std::optional<common::SteadyTime> EventLoop::nextEventTime() {
    purgeCancelled();
    if (tasks_.empty()) {
        return std::nullopt;
    }
    return tasks_.top().time;
}

size_t EventLoop::runDue() {
    isRunningDue_ = true;
    size_t executed = 0;

    while (true) {
        purgeCancelled();
        if (tasks_.empty()) {
            break;
        }

        auto now = clock_.now();
        if (tasks_.top().time > now) {
            break;
        }

        auto task = tasks_.top();
        tasks_.pop();

        // Đo độ trôi (drift)
        auto drift = std::chrono::duration_cast<common::Millis>(now - task.time);
        if (drift > domain::kDriftWarnThreshold && logger_ != nullptr) {
            logger_->warn("EventLoop phát hiện độ trôi cao: " + std::to_string(drift.count()) + " ms");
        }

        try {
            if (task.callback) {
                task.callback();
            }
        } catch (const std::exception& ex) {
            if (logger_ != nullptr) {
                logger_->error(std::string("Lỗi khi thực thi sự kiện EventLoop: ") + ex.what());
            }
        } catch (...) {
            if (logger_ != nullptr) {
                logger_->error("Lỗi không xác định khi thực thi sự kiện EventLoop");
            }
        }
        ++executed;

        // Nếu là sự kiện định kỳ và chưa bị hủy, tính thời điểm tiếp theo = task.time + period
        if (task.period > common::Millis(0) && cancelled_.find(task.id) == cancelled_.end()) {
            task.time += task.period;
            task.sequence = ++sequenceCounter_;
            tasks_.push(std::move(task));
        }
    }

    isRunningDue_ = false;

    if (pendingNotification_) {
        pendingNotification_ = false;
        notifyChanged();
    }

    return executed;
}

void EventLoop::setOnChanged(std::function<void()> onChanged) {
    onChanged_ = std::move(onChanged);
}

void EventLoop::notifyChanged() {
    if (isRunningDue_) {
        pendingNotification_ = true;
        return;
    }
    if (onChanged_) {
        onChanged_();
    }
}

} // namespace auction::infrastructure
