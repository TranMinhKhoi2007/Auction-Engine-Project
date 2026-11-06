#include "auction/infrastructure_qt/QtEventLoopDriver.h"
#include "auction/domain/AuctionConfig.h"
#include <algorithm>
#include <chrono>

namespace auction::infrastructure_qt {

QtEventLoopDriver::QtEventLoopDriver(infrastructure::EventLoop& eventLoop,
                                     const domain::IClock& clock,
                                     QObject* parent)
    : QObject(parent), eventLoop_(eventLoop), clock_(clock), timer_(this) {
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setSingleShot(true);

    connect(&timer_, &QTimer::timeout, this, &QtEventLoopDriver::onTimeout);

    eventLoop_.setOnChanged([this]() {
        if (isProcessing_) {
            needsRearm_ = true;
            return;
        }
        rearm();
    });
}

QtEventLoopDriver::~QtEventLoopDriver() {
    stop();
    eventLoop_.setOnChanged({});
}

void QtEventLoopDriver::start() {
    running_=true;
    rearm();
}

void QtEventLoopDriver::stop() {
    running_=false;
    timer_.stop();
}

void QtEventLoopDriver::onTimeout() {
    isProcessing_ = true;
    needsRearm_ = false;

    eventLoop_.runDue();

    isProcessing_ = false;
    rearm();
}

void QtEventLoopDriver::rearm() {
    timer_.stop();
    if(!running_)return;
    auto next = eventLoop_.nextEventTime();
    if (!next.has_value()) {
        return;
    }

    auto now = clock_.now();
    int64_t delayMs = 0;
    if (*next > now) {
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(*next - now);
        // Làm tròn lên 1ms nếu còn một phần microgiây
        if (std::chrono::duration_cast<std::chrono::microseconds>(*next - now).count() % 1000 > 0) {
            diff += std::chrono::milliseconds(1);
        }
        delayMs = diff.count();
    }

    int64_t maxSleepMs = std::chrono::duration_cast<std::chrono::milliseconds>(domain::kMaxTimerSleep).count();
    delayMs = std::clamp(delayMs, static_cast<int64_t>(0), maxSleepMs);

    timer_.start(static_cast<int>(delayMs));
}

} // namespace auction::infrastructure_qt
