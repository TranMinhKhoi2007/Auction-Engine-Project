#pragma once
#include "auction/client/IAuctionGateway.h"
#include "auction/presentation/QtEventBridge.h"
#include "auction/presentation/state/UserSession.h"

#include <QMap>
namespace auction::presentation {
class AuctionStore : public QObject {
    Q_OBJECT
public:
    AuctionStore(client::IAuctionGateway &gateway, QtEventBridge &bridge, UserSession &session,
                 QObject *parent = nullptr);
    const QMap<quint64, application::AuctionDetailDto> &auctions() const { return auctions_; }
    const application::AuctionDetailDto *get(quint64 id) const;
    const std::vector<application::UserDto> &users() const { return users_; }
    quint64 accountCount() const {return accountCount_;}
    QString userName(common::UserId id) const;
    QString eventText(const domain::AuctionEvent &event) const;
    const std::optional<application::WalletDto> &wallet() const { return wallet_; }
    const std::deque<domain::AuctionEvent> &inbox() const { return inbox_; }
    void refreshAll();
    void refreshAuction(quint64 id);
    void refreshPeople();
    void refreshPersonal();
    common::SteadyTime endSteady(quint64 id) const;
    bool loaded() const {return loaded_;}
    bool busy(quint64 id) const {return busy_.contains(id);}
    bool canWrite(quint64 id=0) const {return gateway_.state()==client::ConnectionState::Connected && !busy(id) && !busy(0);}
    void setBusy(quint64 id,bool value) {if(value)busy_.insert(id);else busy_.remove(id);emit auctionChanged(id);emit personalChanged();}
    bool leaderCanSettle(quint64 id) const {return leaderCanSettle_.value(id);}
signals:
    void auctionChanged(quint64 id);
    void auctionListChanged();
    void peopleChanged();
    void personalChanged();
    void eventProcessed(const domain::AuctionEvent &event);
    void loadingChanged();

private:
    client::IAuctionGateway &gateway_;
    UserSession &session_;
    QMap<quint64, application::AuctionDetailDto> auctions_;
    QSet<quint64> busy_,loading_,dirty_;
    QMap<quint64,common::SteadyTime> deadlines_;
    QMap<quint64,quint64> revisions_;
    QMap<quint64,qint64> stamps_;
    QMap<quint64,bool> leaderCanSettle_;
    QMap<quint64,std::vector<domain::AuctionEvent>> events_;
    quint64 epoch_{},personalEpoch_{},accountCount_{};
    common::UserId personalUser_{};
    bool loaded_{},listLoading_{},listDirty_{},peopleLoading_{},peopleDirty_{},personalLoading_{},personalDirty_{};
    std::vector<application::UserDto> users_;
    std::optional<application::WalletDto> wallet_;
    std::deque<domain::AuctionEvent> inbox_;
};
bool isLive(const application::AuctionDetailDto &a);
bool isClosed(const application::AuctionDetailDto &a);
bool hasJoined(const application::AuctionDetailDto &a, common::UserId id);
QString pendingSchedule(const application::AuctionDetailDto &auction);
QString myStatus(const application::AuctionDetailDto &a, const UserSession &session);
} // namespace auction::presentation
