#pragma once
#include "auction/protocol/ProtocolConfig.h"
#include <QElapsedTimer>
#include <algorithm>
namespace auction::server {
class RateLimiter {
public:
    RateLimiter(int rate=protocol::kRatePerSecond,int burst=protocol::kRateBurst)
        : rate_(rate),burst_(burst),tokens_(burst) { time_.start(); }
    bool consume() {
        const auto elapsed=time_.restart();
        tokens_=std::min(double(burst_),tokens_+double(elapsed)*double(rate_)/1000.0);
        if(tokens_<1.0) return false;
        tokens_-=1.0; return true;
    }
private:
    int rate_,burst_;
    double tokens_;
    QElapsedTimer time_;
};
}
