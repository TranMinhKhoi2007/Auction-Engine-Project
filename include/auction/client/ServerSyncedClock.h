#pragma once
#include "auction/domain/ports/IClock.h"
#include "auction/protocol/ProtocolConfig.h"
#include <deque>
#include <algorithm>
#include <QtTypes>
namespace auction::client {
class ServerSyncedClock final : public domain::IClock {
public:
    explicit ServerSyncedClock(const domain::IClock &local):local_(local) {}
    common::SteadyTime now() const override {return local_.now();}
    common::WallTime wallNow() const override {return local_.wallNow()+offset_;}
    common::WallTime localWallNow() const {return local_.wallNow();}
    void reset() {samples_.clear();offset_=common::Millis(0);}
    void sample(qint64 serverNowMs,common::WallTime sent,common::WallTime received,common::Millis rtt) {
        const auto midpoint=sent+(received-sent)/2;
        const auto offset=common::Millis(serverNowMs)-std::chrono::duration_cast<common::Millis>(midpoint.time_since_epoch());
        samples_.push_back({rtt,offset});if(samples_.size()>static_cast<std::size_t>(protocol::kClockSampleCount)) samples_.pop_front();
        offset_=std::min_element(samples_.begin(),samples_.end(),[](const auto &a,const auto &b){return a.rtt<b.rtt;})->offset;
    }
private:
    struct Sample {common::Millis rtt,offset;};
    const domain::IClock &local_;
    common::Millis offset_{};
    std::deque<Sample> samples_;
};
}
