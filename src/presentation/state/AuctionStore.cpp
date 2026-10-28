#include "auction/presentation/state/AuctionStore.h"

#include "auction/presentation/Formatters.h"

#include <QRegularExpression>
#include <QJsonArray>
#include <algorithm>
namespace auction::presentation {
AuctionStore::AuctionStore(client::IAuctionGateway &gateway, QtEventBridge &bridge,
                           UserSession &session, QObject *parent)
    : QObject(parent), gateway_(gateway), session_(session) {
    connect(&bridge,&QtEventBridge::eventReceived,this,[this](const domain::AuctionEvent &e) {
        const auto id=std::visit([](const auto &v){return v.auctionId.value();},e);
        if(std::holds_alternative<domain::CountdownEvent>(e)) {emit eventProcessed(e);return;}
        ++revisions_[id];events_[id].push_back(e);refreshAuction(id);
        if(std::holds_alternative<domain::ParticipantJoinedEvent>(e) ||
           std::holds_alternative<domain::AuctionClosedEvent>(e) ||
           std::holds_alternative<domain::AuctionCancelledEvent>(e)) refreshPeople();
    });
    connect(&session_,&UserSession::changed,this,[this] {
        if(personalUser_!=session_.id()) {
            personalUser_=session_.id();++personalEpoch_;personalLoading_=false;
            wallet_.reset();inbox_.clear();emit personalChanged();
        }
        refreshPersonal();
    });
    connect(&gateway_,&client::IAuctionGateway::accountCountChanged,this,[this](quint64 count){accountCount_=count;emit peopleChanged();});
    connect(&gateway_,&client::IAuctionGateway::sessionResumed,&session_,[this](const application::AccountDto &account){session_.setUser(account);});
    connect(&gateway_,&client::IAuctionGateway::refreshRequested,this,&AuctionStore::refreshAll);
    connect(&gateway_,&client::IAuctionGateway::clockSynchronized,this,[this] {
        const auto ids=auctions_.keys();for(auto id:ids) {
            const auto &a=auctions_[id];
            if(!isClosed(a) && a.endWallTime)deadlines_[id]=domain::toSteady(gateway_.clock(),*a.endWallTime);
            emit auctionChanged(id);
        }
    });
    connect(&gateway_,&client::IAuctionGateway::connectionStateChanged,this,[this](client::ConnectionState state) {
        if(state==client::ConnectionState::Connected) {
            ++epoch_;loading_.clear();dirty_.clear();stamps_.clear();events_.clear();
            auctions_.clear();deadlines_.clear();leaderCanSettle_.clear();busy_.clear();listLoading_=false;
            peopleLoading_=false;personalLoading_=false;loaded_=false;
            emit auctionListChanged();emit loadingChanged();refreshAll();
        }
    });
}
common::SteadyTime AuctionStore::endSteady(quint64 id) const {
    return deadlines_.value(id);
}
const application::AuctionDetailDto *AuctionStore::get(quint64 id) const {
    auto it = auctions_.constFind(id);
    return it == auctions_.cend() ? nullptr : &it.value();
}
QString AuctionStore::userName(common::UserId id) const {
    for (const auto &u : users_)
        if (u.id == id)
            return QString::fromStdString(u.name);
    return "Người dùng #" + QString::number(id.value());
}
QString AuctionStore::eventText(const domain::AuctionEvent &event) const {
    QString text = Formatters::formatEventSummary(event, session_.id());
    for (const auto &user : users_)
        text.replace(QRegularExpression("\\bU-" + QString::number(user.id.value()) + "\\b"),
                     QString::fromStdString(user.name));
    return text;
}
void AuctionStore::refreshAuction(quint64 id) {
    if(gateway_.state()!=client::ConnectionState::Connected){return;}
    if(loading_.contains(id)) {dirty_.insert(id);return;}
    loading_.insert(id);const auto revision=revisions_.value(id);const auto epoch=epoch_;
    gateway_.getAuctionDetail(common::AuctionId{id},this,[this,id,revision,epoch](auto r) {
        if(epoch!=epoch_){return;}
        loading_.remove(id);
        if(!r) {
            if(auctions_.remove(id)>0) {
                deadlines_.remove(id);revisions_.remove(id);stamps_.remove(id);
                leaderCanSettle_.remove(id);events_.remove(id);
                emit auctionChanged(id);emit auctionListChanged();
            }
        } else if(revision==revisions_.value(id) && r.serverNowMs>=stamps_.value(id)) {
            const bool added=!auctions_.contains(id);
            stamps_[id]=r.serverNowMs;leaderCanSettle_[id]=r.metadata.value("leaderCanSettle").toBool();
            deadlines_[id]=!isClosed(*r.value) && r.value->endWallTime?domain::toSteady(gateway_.clock(),*r.value->endWallTime):gateway_.clock().now();
            for(const auto &value:r.metadata.value("participantNames").toArray()) {
                const auto person=value.toObject();const auto userId=common::UserId{quint64(person.value("id").toDouble())};
                const auto found=std::find_if(users_.begin(),users_.end(),[&](const auto &u){return u.id==userId;});
                if(found==users_.end()) {application::UserDto user{};user.id=userId;user.name=person.value("name").toString().toStdString();users_.push_back(std::move(user));}
                else found->name=person.value("name").toString().toStdString();
            }
            emit peopleChanged();
            auctions_.insert(id,std::move(*r.value));emit auctionChanged(id);if(added)emit auctionListChanged();
            const auto events=events_.take(id);for(const auto &e:events)emit eventProcessed(e);
        }
        if(dirty_.remove(id)>0)refreshAuction(id);
        if(!loaded_ && !listLoading_ && loading_.isEmpty()) {loaded_=true;emit loadingChanged();}
    });
}
void AuctionStore::refreshPeople() {
    if(gateway_.state()!=client::ConnectionState::Connected){return;}
    if(peopleLoading_) {peopleDirty_=true;return;}
    peopleLoading_=true;const auto epoch=epoch_;
    gateway_.request("sys.ping",{},this,[this,epoch](auto r) {
        if(epoch!=epoch_){return;}peopleLoading_=false;
        if(r)emit peopleChanged();
        if(peopleDirty_){peopleDirty_=false;refreshPeople();}
    });
    refreshPersonal();
}
void AuctionStore::refreshPersonal() {
    if(!session_.user) {wallet_.reset();inbox_.clear();personalLoading_=false;emit personalChanged();return;}
    if(gateway_.state()!=client::ConnectionState::Connected){return;}
    if(personalLoading_){personalDirty_=true;return;}
    personalLoading_=true;personalDirty_=false;
    const auto id=session_.id();const auto epoch=personalEpoch_;
    auto remaining=std::make_shared<int>(3);
    const auto finish=[this,remaining,epoch] {
        if(epoch!=personalEpoch_){return;}
        if(--*remaining==0){personalLoading_=false;emit personalChanged();if(personalDirty_){personalDirty_=false;refreshPersonal();}}
    };
    gateway_.getWallet(this,[this,id,epoch,finish](auto r) {
        if(epoch==personalEpoch_ && id==session_.id() && r){wallet_=std::move(*r.value);}finish();
    });
    gateway_.getUser(this,[this,id,epoch,finish](auto r) {
        if(epoch==personalEpoch_ && id==session_.id() && r){session_.user->name=r.value->name;}finish();
    });
    gateway_.getInbox(this,[this,id,epoch,finish](auto r) {
        if(epoch==personalEpoch_ && id==session_.id() && r){inbox_=std::move(*r.value);}finish();
    });
}
void AuctionStore::refreshAll() {
    if(gateway_.state()!=client::ConnectionState::Connected){return;}
    refreshPeople();
    if(listLoading_){listDirty_=true;return;}
    listLoading_=true;const auto epoch=epoch_;emit loadingChanged();
    gateway_.listAuctions(this,[this,epoch](auto r) {
        if(epoch!=epoch_){return;}listLoading_=false;
        if(r)for(const auto &a:*r.value)refreshAuction(a.id.value());
        if(loading_.isEmpty()){loaded_=true;emit loadingChanged();}
        if(listDirty_){listDirty_=false;refreshAll();}
    });
}
bool isLive(const application::AuctionDetailDto &a) {
    return a.state == "Đang diễn ra";
}
bool isClosed(const application::AuctionDetailDto &a) {
    return a.state == "Kết thúc" || a.state == "Hủy";
}
bool hasJoined(const application::AuctionDetailDto &a, common::UserId id) {
    return std::find(a.participants.begin(), a.participants.end(), id) != a.participants.end();
}
QString pendingSchedule(const application::AuctionDetailDto &a) {
    return "Chưa có lịch · bắt đầu sau " + QString::number(a.startDelayMs.count() / 1000) +
           " giây kể từ khi duyệt, kéo dài " + QString::number(a.durationMs.count() / 1000) + " giây";
}
QString myStatus(const application::AuctionDetailDto &a, const UserSession &s) {
    if (a.state == "Hủy")
        return "Phiên đã bị hủy, không thể đặt giá.";
    if (a.state == "Kết thúc") {
        if (a.leaderId == s.id())
            return a.outcome == domain::AuctionOutcome::Sold
                       ? "Bạn đã thắng phiên đấu giá"
                       : "Bạn không thanh toán — tiền cọc đã bị tịch thu";
        return hasJoined(a, s.id()) ? "Phiên kết thúc · Bạn được hoàn tiền cọc"
                                    : "Phiên đã kết thúc, không thể đặt giá.";
    }
    if (!s.user)
        return "Đăng nhập để tham gia.";
    if (a.sellerId == s.id())
        return "Bạn là người bán";
    if (a.leaderId == s.id())
        return "Bạn đang dẫn đầu";
    const bool bid = std::any_of(a.bidHistory.begin(), a.bidHistory.end(),
                                 [&](const auto &b) { return b.bidderId() == s.id(); });
    if (bid)
        return "Bạn đã bị vượt giá";
    if (hasJoined(a, s.id()))
        return "Bạn đã tham gia · Đã đặt cọc";
    if (a.state == "Chờ duyệt")
        return "Phiên đang chờ chủ phiên duyệt.";
    return "Bạn chưa tham gia · Đặt cọc để bắt đầu";
}
} // namespace auction::presentation
