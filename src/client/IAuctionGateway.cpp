#include "auction/client/IAuctionGateway.h"
#include "auction/protocol/JsonCodec.h"
#include "auction/domain/values/SecretBuffer.h"
namespace auction::client {
namespace {
template<class T,class Decode> Callback<QJsonValue> converted(Callback<T> callback,Decode decode) {
    return [callback=std::move(callback),decode=std::move(decode)](GatewayResult<QJsonValue> r) {
        GatewayResult<T> result;result.error=r.error;result.serverNowMs=r.serverNowMs;result.metadata=r.metadata;
        if(r.value && !r.error) {
            try {result.value=decode(*r.value);} catch(const std::exception &) {result.error=GatewayError{"BadRequest",{},"Phản hồi máy chủ không hợp lệ",{}};}
        }
        callback(std::move(result));
    };
}
}
void IAuctionGateway::getUser(QObject *context, Callback<application::UserDto> callback) {request("auth.getUser",{},context,converted<application::UserDto>(std::move(callback),[](const QJsonValue &v) {return protocol::fromJson<application::UserDto>(v);}));}
void IAuctionGateway::getWallet(QObject *context, Callback<application::WalletDto> callback) {request("wallet.get",{},context,converted<application::WalletDto>(std::move(callback),[](const QJsonValue &v) {return protocol::fromJson<application::WalletDto>(v);}));}
void IAuctionGateway::getInbox(QObject *context, Callback<std::deque<domain::AuctionEvent>> callback) {request("inbox.get",{},context,converted<std::deque<domain::AuctionEvent>>(std::move(callback),[](const QJsonValue &v) {return [&] {std::deque<domain::AuctionEvent> events; for(const auto &e:protocol::readArray(v)) events.push_back(protocol::fromJson<domain::AuctionEvent>(e)); return events;}();}));}
void IAuctionGateway::listAuctions(QObject *context, Callback<std::vector<application::AuctionSummaryDto>> callback) {request("auction.list",{},context,converted<std::vector<application::AuctionSummaryDto>>(std::move(callback),[](const QJsonValue &v) {return [&] {std::vector<application::AuctionSummaryDto> auctions; for(const auto &e:protocol::readArray(v)) auctions.push_back(protocol::fromJson<application::AuctionSummaryDto>(e)); return auctions;}();}));}
void IAuctionGateway::getAuctionDetail(common::AuctionId id, QObject *context, Callback<application::AuctionDetailDto> callback) {request("auction.get",{{"auctionId",double(id.value())}},context,converted<application::AuctionDetailDto>(std::move(callback),[](const QJsonValue &v) {return protocol::fromJson<application::AuctionDetailDto>(v);}));}
void IAuctionGateway::registerUser(application::RegisterUserCommand command, QObject *context, Callback<application::AccountDto> callback) {domain::SecretBuffer wipe(command.password);request("auth.register",protocol::toJson(command),context,converted<application::AccountDto>(std::move(callback),[](const QJsonValue &v) {return protocol::fromJson<application::AccountDto>(protocol::readObject(v).value("account"));}));}
void IAuctionGateway::authenticate(std::string email, std::string password, QObject *context, Callback<application::AccountDto> callback) {domain::SecretBuffer wipe(password);request("auth.login",{{"email",QString::fromStdString(email)},{"password",QString::fromStdString(password)}},context,converted<application::AccountDto>(std::move(callback),[](const QJsonValue &v) {return protocol::fromJson<application::AccountDto>(protocol::readObject(v).value("account"));}));}
void IAuctionGateway::resetPassword(std::string email, std::string newPassword, QObject *context, Callback<application::AccountDto> callback) {domain::SecretBuffer wipe(newPassword);request("auth.resetPassword",{{"email",QString::fromStdString(email)},{"newPassword",QString::fromStdString(newPassword)}},context,converted<application::AccountDto>(std::move(callback),[](const QJsonValue &v) {return protocol::fromJson<application::AccountDto>(protocol::readObject(v).value("account"));}));}
void IAuctionGateway::changePassword(std::string currentPassword, std::string newPassword, QObject *context, Callback<QJsonObject> callback) {
    domain::SecretBuffer wipeOld(currentPassword);
    domain::SecretBuffer wipeNew(newPassword);
    request("auth.changePassword", {{"currentPassword", QString::fromStdString(currentPassword)}, {"newPassword", QString::fromStdString(newPassword)}}, context, converted<QJsonObject>(std::move(callback), [](const QJsonValue &v) { return protocol::readObject(v); }));
}
void IAuctionGateway::logout(QObject *context, Callback<QJsonObject> callback) {request("auth.logout",{},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::topUp(domain::Money amount, QObject *context, Callback<application::WalletDto> callback) {request("wallet.topUp",{{"amount",QString::number(amount.amount())}},context,converted<application::WalletDto>(std::move(callback),[](const QJsonValue &v) {return protocol::fromJson<application::WalletDto>(v);}));}
void IAuctionGateway::createAuction(application::CreateAuctionCommand command, QObject *context, Callback<common::AuctionId> callback) {request("auction.create",protocol::toJson(command),context,converted<common::AuctionId>(std::move(callback),[](const QJsonValue &v) {return common::AuctionId{protocol::readId(protocol::readObject(v).value("id"))};}));}
void IAuctionGateway::approveAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback) {request("auction.approve",{{"auctionId",double(id.value())}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::closeAuctionEarly(common::AuctionId id, QObject *context, Callback<QJsonObject> callback) {request("auction.closeEarly",{{"auctionId",double(id.value())}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::followAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback) {request("auction.follow",{{"auctionId",double(id.value())}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::unfollowAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback) {request("auction.unfollow",{{"auctionId",double(id.value())}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::joinAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback) {request("auction.join",{{"auctionId",double(id.value())}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::cancelAuction(common::AuctionId id, std::string reason, QObject *context, Callback<QJsonObject> callback) {request("auction.cancel",{{"auctionId",double(id.value())},{"reason",QString::fromStdString(reason)}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::deleteAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback) {request("auction.delete",{{"auctionId",double(id.value())}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
void IAuctionGateway::placeBid(common::AuctionId id, domain::Money amount, QObject *context, Callback<QJsonObject> callback) {request("auction.placeBid",{{"auctionId",double(id.value())},{"amount",QString::number(amount.amount())}},context,converted<QJsonObject>(std::move(callback),[](const QJsonValue &v) {return protocol::readObject(v);}));}
}
