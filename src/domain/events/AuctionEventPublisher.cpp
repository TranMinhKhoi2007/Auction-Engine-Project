#include "auction/domain/events/AuctionEventPublisher.h"
#include <algorithm>

namespace auction::domain {

void AuctionEventPublisher::subscribe(std::weak_ptr<IAuctionObserver> observer) {
    if (observer.expired()) {
        return;
    }
    // Dọn các weak_ptr đã hết hạn trước khi thêm
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
                       [](const std::weak_ptr<IAuctionObserver>& wp) { return wp.expired(); }),
        observers_.end());

    observers_.push_back(std::move(observer));
}

void AuctionEventPublisher::unsubscribe(const std::shared_ptr<IAuctionObserver>& observer) {
    if (!observer) {
        return;
    }
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
                       [&observer](const std::weak_ptr<IAuctionObserver>& wp) {
                           auto sp = wp.lock();
                           return !sp || sp == observer;
                       }),
        observers_.end());
}

void AuctionEventPublisher::publish(const AuctionEvent& event) {
    // Tạo bản sao danh sách đang active để an toàn khi callback gọi subscribe/unsubscribe
    std::vector<std::shared_ptr<IAuctionObserver>> active;
    active.reserve(observers_.size());

    // Thu dọn các weak_ptr đã expired và trích xuất shared_ptr
    auto it = observers_.begin();
    while (it != observers_.end()) {
        if (auto sp = it->lock()) {
            active.push_back(std::move(sp));
            ++it;
        } else {
            it = observers_.erase(it);
        }
    }

    // Gửi sự kiện cho từng observer, cô lập lỗi nếu observer ném exception
    for (const auto& observer : active) {
        try {
            observer->onAuctionEvent(event);
        } catch (...) {
            // Không để exception của một observer làm sập hệ thống hay ảnh hưởng observer khác
        }
    }
}

size_t AuctionEventPublisher::observerCount() const {
    size_t count = 0;
    for (const auto& wp : observers_) {
        if (!wp.expired()) {
            ++count;
        }
    }
    return count;
}

} // namespace auction::domain
