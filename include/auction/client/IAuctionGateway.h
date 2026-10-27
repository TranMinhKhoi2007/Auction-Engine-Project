#pragma once
#include "auction/application/Dtos.h"
#include "auction/application/Commands.h"
#include "auction/domain/ports/IClock.h"
#include "auction/client/GatewayError.h"
#include "auction/client/ConnectionState.h"
#include <QObject>
#include <QUrl>
#include <deque>
namespace auction::client {
class IAuctionGateway : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual const domain::IClock &clock() const=0;
    virtual ConnectionState state() const=0;
    virtual void connectToServer(QUrl url)=0;
    virtual QUrl serverUrl() const=0;
    virtual void setCaCertificateFile(QString path)=0;
    virtual QString caCertificateFile() const=0;
    virtual void retryNow()=0;
    virtual void request(QString method,QJsonObject params,QObject *context,Callback<QJsonValue> callback)=0;
    void getUser(QObject *context, Callback<application::UserDto> callback);
    void getWallet(QObject *context, Callback<application::WalletDto> callback);
    void getInbox(QObject *context, Callback<std::deque<domain::AuctionEvent>> callback);
    void listAuctions(QObject *context, Callback<std::vector<application::AuctionSummaryDto>> callback);
    void getAuctionDetail(common::AuctionId id, QObject *context, Callback<application::AuctionDetailDto> callback);
    void registerUser(application::RegisterUserCommand command, QObject *context, Callback<application::AccountDto> callback);
    void authenticate(std::string email, std::string password, QObject *context, Callback<application::AccountDto> callback);
    void resetPassword(std::string email, std::string newPassword, QObject *context, Callback<application::AccountDto> callback);
    void changePassword(std::string currentPassword, std::string newPassword, QObject *context, Callback<QJsonObject> callback);
    void logout(QObject *context, Callback<QJsonObject> callback);
    void topUp(domain::Money amount, QObject *context, Callback<application::WalletDto> callback);
    void createAuction(application::CreateAuctionCommand command, QObject *context, Callback<common::AuctionId> callback);
    void approveAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback);
    void closeAuctionEarly(common::AuctionId id, QObject *context, Callback<QJsonObject> callback);
    void followAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback);
    void unfollowAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback);
    void joinAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback);
    void cancelAuction(common::AuctionId id, std::string reason, QObject *context, Callback<QJsonObject> callback);
    void deleteAuction(common::AuctionId id, QObject *context, Callback<QJsonObject> callback);
    void placeBid(common::AuctionId id, domain::Money amount, QObject *context, Callback<QJsonObject> callback);
signals:
    void eventReceived(const domain::AuctionEvent &event);
    void connectionStateChanged(ConnectionState state);
    void sessionEnded(const QString &reason);
    void sessionResumed(const application::AccountDto &account);
    void forbidden();
    void accountCountChanged(quint64 count);
    void refreshRequested();
    void clockSynchronized();
    void clientLog(const QString &level,const QString &message);
    void connectionProblem(const QString &message);
};
}
