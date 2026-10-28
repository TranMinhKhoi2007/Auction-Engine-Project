#include "auction/server/RpcDispatcher.h"
#include "auction/media/ProductImage.h"
#include "auction/protocol/JsonCodec.h"
#include "auction/domain/exceptions/AuctionException.h"
#include <algorithm>
#include <set>
#include "auction/server/ServerRuntime.h"
#include "auction/server/AuthorizationPolicy.h"
#include "auction/domain/exceptions/AuthorizationException.h"
#include "auction/domain/values/SecretBuffer.h"
namespace auction::server {
using namespace protocol;
namespace {
QString reasonName(domain::BidRejectReason reason) {
    switch(reason) {
    case domain::BidRejectReason::AuctionNotActive:return "AuctionNotActive";
    case domain::BidRejectReason::BelowStartingPrice:return "BelowStartingPrice";
    case domain::BidRejectReason::BelowMinimumIncrement:return "BelowMinimumIncrement";
    case domain::BidRejectReason::InsufficientFunds:return "InsufficientFunds";
    case domain::BidRejectReason::SellerCannotBid:return "SellerCannotBid";
    case domain::BidRejectReason::AuctionExpired:return "AuctionExpired";
    case domain::BidRejectReason::DepositNotPaid:return "DepositNotPaid";
    case domain::BidRejectReason::AlreadyLeading:return "AlreadyLeading";
    }
    return "Unknown";
}
common::AuctionId auctionId(const QJsonObject &p) {return common::AuctionId{readId(p.value("auctionId"))};}
template<class T> const T &required(const std::optional<T> &v) { if(!v) throw ProtocolError("Không tìm thấy dữ liệu","EntityNotFound"); return *v; }
void validateTimeline(const domain::IClock &clock,common::Millis delay,common::Millis duration) {
    if(delay.count()<0 || duration.count()<0)return; // semantic minima remain the core's responsibility.
    const auto wallBudget=std::chrono::duration_cast<common::Millis>(common::WallTime::max().time_since_epoch()).count()-std::chrono::duration_cast<common::Millis>(clock.wallNow().time_since_epoch()).count();
    const auto steadyBudget=std::chrono::duration_cast<common::Millis>(common::SteadyTime::max().time_since_epoch()).count()-std::chrono::duration_cast<common::Millis>(clock.now().time_since_epoch()).count();
    const auto budget=std::min({wallBudget,steadyBudget,std::chrono::duration_cast<common::Millis>(common::SteadyTime::duration::max()).count()});
    if(delay.count()>budget || duration.count()>budget-delay.count())throw ProtocolError("Lịch vượt phạm vi đồng hồ");
}
}
RpcDispatcher::RpcDispatcher(ServerRuntime &runtime,SessionRegistry &registry,PasswordWorker &passwords)
    :runtime_(runtime),registry_(registry),passwords_(passwords),engine_(runtime.engine()),logger_(runtime.logger()) {
    methods_["sys.hello"]=Sync{[this](ClientConnection &c,const std::optional<AuthContext> &,QJsonObject p)->QJsonValue {
        if(readInteger(p.value("protocolVersion"))!=kVersion)throw ProtocolError("Phiên bản không tương thích","ProtocolMismatch");
        c.clientName=readString(p.value("clientName"));c.hello=true;
        const auto now=std::chrono::duration_cast<common::Millis>(engine_.clock().wallNow().time_since_epoch()).count();
        return QJsonObject{{"protocolVersion",kVersion},{"serverVersion","2.0.0"},{"accountCount",double(engine_.listUsers().size())},{"serverNowMs",now}};
    }};
    methods_["sys.ping"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &,QJsonObject)->QJsonValue {
        return QJsonObject{{"accountCount",double(engine_.listUsers().size())},{"serverNowMs",std::chrono::duration_cast<common::Millis>(engine_.clock().wallNow().time_since_epoch()).count()}};
    }};
    methods_["auth.register"]=Async{[this](ClientConnection &c,const std::optional<AuthContext> &,QJsonObject p,Completion done){registerAccount(c,std::move(p),std::move(done));}};
    methods_["auth.login"]=Async{[this](ClientConnection &c,const std::optional<AuthContext> &,QJsonObject p,Completion done){login(c,std::move(p),std::move(done));}};
    methods_["auth.resetPassword"]=Async{[this](ClientConnection &c,const std::optional<AuthContext> &,QJsonObject p,Completion done){resetPassword(c,std::move(p),std::move(done));}};
    methods_["auth.changePassword"]=Async{[this](ClientConnection &c,const std::optional<AuthContext> &auth,QJsonObject p,Completion done){changePassword(c,auth,std::move(p),std::move(done));}};
    methods_["auth.resume"]=Sync{[this](ClientConnection &c,const std::optional<AuthContext> &,QJsonObject p)->QJsonValue {
        registry_.purgeExpired();auto token=readString(p.value("sessionToken")).toStdString();domain::SecretBuffer wipe(token);
        const auto session=runtime_.sessions().validate(token);
        if(!session)throw ProtocolError("Phiên đăng nhập đã hết hạn","Unauthorized","SessionExpired");
        registry_.bind(c,*session);runtime_.security().info("SESSION_RESUMED user="+session->userId.toString());
        return QJsonObject{{"account",toJson(engine_.accountOf(session->userId))},{"expiresAtMs",std::chrono::duration_cast<common::Millis>(runtime_.sessions().expiresAt(session->sessionId).time_since_epoch()).count()}};
    }};
    methods_["auth.logout"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject)->QJsonValue {
        registry_.apply(runtime_.sessions().revoke(auth->sessionId,application::RevocationReason::LoggedOut));return QJsonObject{};
    }};
    methods_["auth.getUser"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject) -> QJsonValue {return toJson(required(engine_.getUser(auth->userId)));}};
    methods_["wallet.get"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject) -> QJsonValue {return toJson(required(engine_.getWallet(auth->userId)));}};
    methods_["wallet.topUp"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.topUp(auth->userId,readMoney(p.value("amount"))); return toJson(required(engine_.getWallet(auth->userId)));}};
    methods_["inbox.get"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject) -> QJsonValue {
        QJsonArray a; for(const auto &e:engine_.getInbox(auth->userId)) a.append(toJson(e)); return a;
    }};
    methods_["auction.list"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &,QJsonObject) -> QJsonValue {
        QJsonArray a; for(const auto &dto:engine_.listAuctions()) a.append(toJson(dto)); return a;
    }};
    methods_["auction.get"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {
        const auto detail=required(engine_.getAuctionDetail(auctionId(p))); auto o=toJson(detail);
        if(auth && auth->userId==detail.sellerId && detail.leaderId) {
            const auto w=required(engine_.getWallet(*detail.leaderId)); auto held=domain::Money::zero();
            for(const auto &h:w.holds) if(h.auctionId==detail.id) held=h.amount;
            o.insert("leaderCanSettle",w.available.amount()>=detail.currentPrice.amount()-held.amount());
        }
        std::set<common::UserId> ids(detail.participants.begin(),detail.participants.end());ids.insert(detail.sellerId);
        QJsonArray names;for(auto id:ids)if(auto user=engine_.getUser(id))names.append(QJsonObject{{"id",static_cast<double>(id.value())},{"name",QString::fromStdString(user->name)}});
        o.insert("participantNames",names);return o;
    }};
    methods_["auction.image"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &,QJsonObject p)->QJsonValue {
        return QJsonObject{{"imageBase64",imageBase64(required(engine_.getAuctionImage(auctionId(p))))}};
    }};
    methods_["auction.create"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {
        // Overwrite both identities before decoding; spoofed values and types are ignored.
        p.insert("sellerId",static_cast<double>(auth->userId.value()));
        auto product=readObject(p.value("productCommand")); product.insert("sellerId",static_cast<double>(auth->userId.value())); p.insert("productCommand",product);
        const auto command=fromJson<application::CreateAuctionCommand>(p);
        if(!command.imageData.empty()) {
            try {(void)media::decodeProductImage(QByteArray::fromStdString(command.imageData));}
            catch(const std::exception &) {throw ProtocolError("Không đọc được ảnh sản phẩm. Hãy chọn ảnh khác.","Validation","InvalidImage");}
        }
        if(static_cast<int>(command.productCommand.type)<0) throw ProtocolError("Loại sản phẩm không hỗ trợ","Validation");
        validateTimeline(engine_.clock(),std::chrono::duration_cast<common::Millis>(command.startDelay),std::chrono::duration_cast<common::Millis>(command.duration));
        return QJsonObject{{"id",static_cast<double>(engine_.createAuction(command).value())}};
    }};
    methods_["auction.approve"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {
        const auto id=auctionId(p);const auto dto=engine_.getAuctionDetail(id);
        if(dto && dto->sellerId==auth->userId && dto->state=="Chờ duyệt")validateTimeline(engine_.clock(),dto->startDelayMs,dto->durationMs);
        engine_.approveAuction(id,auth->userId);return QJsonObject{};
    }};
    methods_["auction.cancel"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.cancelAuction(auctionId(p),auth->userId,readString(p.value("reason")).toStdString());return QJsonObject{};}};
    methods_["auction.delete"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.deleteAuction(auctionId(p),auth->userId);return QJsonObject{};}};
    methods_["auction.closeEarly"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.closeAuctionEarly(auctionId(p),auth->userId);return QJsonObject{};}};
    methods_["auction.follow"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.followAuction(auctionId(p),auth->userId);return QJsonObject{};}};
    methods_["auction.unfollow"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.unfollowAuction(auctionId(p),auth->userId);return QJsonObject{};}};
    methods_["auction.join"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.joinAuction(auctionId(p),auth->userId);return QJsonObject{};}};
    methods_["auction.placeBid"]=Sync{[this](ClientConnection &,const std::optional<AuthContext> &auth,QJsonObject p) -> QJsonValue {engine_.placeBid(auctionId(p),auth->userId,readMoney(p.value("amount")));return QJsonObject{};}};
}
std::vector<std::string> RpcDispatcher::registeredMethods() const {
    std::vector<std::string> names;for(const auto &[method,handler]:methods_){(void)handler;names.push_back(method.toStdString());}return names;
}
void RpcDispatcher::logForbidden(const ClientConnection &c,const Request &r) {
    const auto *policy=AuthorizationPolicy::find(r.method.toStdString());
    std::string line="FORBIDDEN method="+(policy?std::string(policy->method):"<unknown>");
    if(auto auth=registry_.context(c))line+=" user="+auth->userId.toString();
    try {if(r.params.contains("auctionId"))line+=" auction="+std::to_string(readId(r.params.value("auctionId")));}catch(const ProtocolError &){}
    runtime_.security().warn(line);
}
DispatchResult RpcDispatcher::dispatch(ClientConnection &c,const Request &r,Completion done) {
    const auto *policy=AuthorizationPolicy::find(r.method.toStdString());
    if(!policy){logForbidden(c,r);throw ProtocolError("Bạn không có quyền thực hiện thao tác này","Forbidden");}
    if(!c.hello && r.method!="sys.hello")throw ProtocolError("Cần sys.hello trước");
    if(c.auth && !runtime_.sessions().isValid(c.auth->sessionId))registry_.apply(runtime_.sessions().revoke(c.auth->sessionId,application::RevocationReason::Expired));
    const auto auth=registry_.context(c);
    if(policy->requiresSession && !auth) {
        runtime_.security().warn("UNAUTHORIZED method="+std::string(policy->method));
        const auto reason=c.lastSessionReason=="Expired" || c.lastSessionReason=="Evicted"?"SessionExpired":"NotLoggedIn";
        throw ProtocolError("Vui lòng đăng nhập","Unauthorized",reason);
    }
    if(auth)(void)runtime_.sessions().touch(auth->sessionId);
    if(c.authPending && (r.method=="auth.register" || r.method=="auth.login" || r.method=="auth.resume" || r.method=="auth.logout"))throw ProtocolError("Đang xử lý xác thực, hãy thử lại","Busy");
    const auto it=methods_.find(r.method);
    if(it==methods_.end()){logForbidden(c,r);throw ProtocolError("Bạn không có quyền thực hiện thao tác này","Forbidden");}
    try {
        if(const auto *handler=std::get_if<Sync>(&it->second))return (*handler)(c,auth,r.params);
        std::get<Async>(it->second)(c,auth,r.params,std::move(done));return Deferred{};
    } catch(const domain::AuthorizationException &) {logForbidden(c,r);throw;}
      catch(const domain::InvalidBidException &e) {if(e.reason()==domain::BidRejectReason::SellerCannotBid)logForbidden(c,r);throw;}
      catch(const ProtocolError &e) {if(e.type=="Unauthorized")runtime_.security().warn("UNAUTHORIZED method="+std::string(policy->method));throw;}
}
ErrorPayload RpcDispatcher::mapException(const std::exception &e,const Request &r) {
    const auto message=QString::fromUtf8(e.what());
    if(const auto *p=dynamic_cast<const ProtocolError *>(&e)) return {p->type,p->reason,message,p->data};
    if(dynamic_cast<const domain::AuthorizationException *>(&e))return {"Forbidden",{},message,{}};
    if(const auto *p=dynamic_cast<const domain::AccountValidationException *>(&e))return {"Validation",QString::fromStdString(p->reason()),message,{}};
    if(const auto *p=dynamic_cast<const domain::InvalidBidException *>(&e)) {
        if(p->reason()==domain::BidRejectReason::SellerCannotBid)return {"Forbidden","SellerCannotBid",message,{}};
        QJsonObject data;
        try { if(auto d=engine_.getAuctionDetail(auctionId(r.params))) data.insert("minimumNextBid",QString::number(d->minimumNextBid.amount())); } catch(...) {}
        return {"InvalidBid",reasonName(p->reason()),message,data};
    }
    if(const auto *p=dynamic_cast<const domain::InsufficientBalanceException *>(&e)) return {"InsufficientBalance",{},message,{{"required",QString::number(p->required())},{"available",QString::number(p->available())}}};
    if(const auto *p=dynamic_cast<const domain::InvalidStateTransitionException *>(&e)) return {"InvalidStateTransition",{},message,{{"currentState",stateName(p->currentState())},{"action",QString::fromStdString(p->action())}}};
    if(const auto *p=dynamic_cast<const domain::EntityNotFoundException *>(&e)) return {"EntityNotFound",{},message,{{"entityType",QString::fromStdString(p->entityType())},{"id",static_cast<double>(p->id())}}};
    if(dynamic_cast<const domain::AuthenticationException *>(&e)) return {"Authentication",{},message,{}};
    if(dynamic_cast<const domain::ValidationException *>(&e)) return {"Validation",{},message,{}};
    logger_.error("Internal: "+std::string(e.what())); return {"Internal",{},"Lỗi hệ thống",{}};
}
}
