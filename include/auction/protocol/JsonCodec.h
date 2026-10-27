#pragma once
#include "auction/application/Dtos.h"
#include "auction/application/Commands.h"
#include "auction/protocol/Errors.h"
#include <QJsonArray>
#include <QJsonObject>
namespace auction::protocol {
quint64 readId(const QJsonValue &v);
qint64 readInteger(const QJsonValue &v);
QString readString(const QJsonValue &v);
QString imageBase64(const std::string &bytes);
std::string readImageData(const QJsonValue &value);
QJsonObject readObject(const QJsonValue &v);
QJsonArray readArray(const QJsonValue &v);
domain::Money readMoney(const QJsonValue &v);
template<class T> T fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::LuotTraGia &value);
template<> domain::LuotTraGia fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::AuctionEvent &value);
template<> domain::AuctionEvent fromJson(const QJsonValue &value);
QJsonObject toJson(const ErrorPayload &value);
template<> ErrorPayload fromJson(const QJsonValue &value);
QJsonObject toJson(const application::UserDto &value);
QJsonObject toJson(const application::AccountDto &value);
template<> application::AccountDto fromJson(const QJsonValue &value);
template<> application::UserDto fromJson(const QJsonValue &value);
QJsonObject toJson(const application::AuctionSummaryDto &value);
template<> application::AuctionSummaryDto fromJson(const QJsonValue &value);
QJsonObject toJson(const application::WalletHoldEntry &value);
template<> application::WalletHoldEntry fromJson(const QJsonValue &value);
QJsonObject toJson(const application::WalletDto &value);
template<> application::WalletDto fromJson(const QJsonValue &value);
QJsonObject toJson(const application::AuctionDetailDto &value);
template<> application::AuctionDetailDto fromJson(const QJsonValue &value);
QJsonObject toJson(const application::CreateProductCommand &value);
template<> application::CreateProductCommand fromJson(const QJsonValue &value);
QJsonObject toJson(const application::CreateAuctionCommand &value);
template<> application::CreateAuctionCommand fromJson(const QJsonValue &value);
QJsonObject toJson(const application::RegisterUserCommand &value);
template<> application::RegisterUserCommand fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::BidPlacedEvent &value);
template<> domain::BidPlacedEvent fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::OutbidEvent &value);
template<> domain::OutbidEvent fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::ParticipantJoinedEvent &value);
template<> domain::ParticipantJoinedEvent fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::StateChangedEvent &value);
template<> domain::StateChangedEvent fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::CountdownEvent &value);
template<> domain::CountdownEvent fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::AuctionClosedEvent &value);
template<> domain::AuctionClosedEvent fromJson(const QJsonValue &value);
QJsonObject toJson(const domain::AuctionCancelledEvent &value);
template<> domain::AuctionCancelledEvent fromJson(const QJsonValue &value);
QString outcomeName(domain::AuctionOutcome value);
QString stateName(const std::string &value);
}
