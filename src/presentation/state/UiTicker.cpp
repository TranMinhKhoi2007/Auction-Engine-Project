#include "auction/presentation/state/UiTicker.h"

#include "auction/domain/AuctionConfig.h"
namespace auction::presentation {
UiTicker::UiTicker(const domain::IClock &clock, QObject *parent) : QObject(parent), clock_(clock) {
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(static_cast<int>(domain::kUiRefreshInterval.count()));
    connect(&timer_, &QTimer::timeout, this, [this] { emit tick(clock_.wallNow(), clock_.now()); });
    timer_.start();
}
} // namespace auction::presentation
