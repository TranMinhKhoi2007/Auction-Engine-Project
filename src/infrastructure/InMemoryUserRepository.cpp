#include "auction/infrastructure/InMemoryUserRepository.h"
#include "auction/domain/entities/NguoiDung.h"

namespace auction::infrastructure {

void InMemoryUserRepository::save(std::shared_ptr<domain::NguoiDung> user) {
    if (!user) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto existing=emails_.find(user->email());
    if(existing!=emails_.end() && existing->second!=user->id())
        throw domain::AccountValidationException("EmailTaken","Email đã được đăng ký");
    if(const auto old=users_.find(user->id());old!=users_.end() && old->second->email()!=user->email())
        emails_.erase(old->second->email());
    emails_.insert_or_assign(user->email(),user->id());
    users_[user->id()] = std::move(user);
}

std::shared_ptr<domain::NguoiDung> InMemoryUserRepository::findById(common::UserId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it != users_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<domain::NguoiDung>> InMemoryUserRepository::findAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<domain::NguoiDung>> list;
    list.reserve(users_.size());
    for (const auto& [_, user] : users_) {
        list.push_back(user);
    }
    return list;
}

std::shared_ptr<domain::NguoiDung> InMemoryUserRepository::findByEmail(const domain::Email &email) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it=emails_.find(email);
    return it==emails_.end()?nullptr:users_.at(it->second);
}
bool InMemoryUserRepository::emailExists(const domain::Email &email) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return emails_.contains(email);
}
} // namespace auction::infrastructure
