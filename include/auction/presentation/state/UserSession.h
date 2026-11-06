#pragma once
#include "auction/application/Dtos.h"
#include "auction/domain/values/SecretBuffer.h"

#include <QMap>
#include <QObject>
#include <QSet>
namespace auction::presentation {
class UserSession : public QObject {
    Q_OBJECT
public:
    explicit UserSession(QObject *parent = nullptr) : QObject(parent) {}
    ~UserSession() override {if(user){domain::SecretBuffer name(user->name),email(user->email);}}
    std::optional<application::AccountDto> user;
    common::UserId id() const { return user ? user->id : common::UserId{}; }
    bool loggedIn() const { return user.has_value(); }
    void setUser(std::optional<application::AccountDto> value) {
        if(user) {domain::SecretBuffer name(user->name),email(user->email);}
        user = std::move(value);
        emit changed();
    }
    bool follows(quint64 auction) const { return followed.value(id().value()).contains(auction); }
    void setFollowed(quint64 auction, bool value) {
        if (value)
            followed[id().value()].insert(auction);
        else
            followed[id().value()].remove(auction);
        emit changed();
    }
signals:
    void changed();

private:
    QMap<quint64, QSet<quint64>> followed;
};
} // namespace auction::presentation
