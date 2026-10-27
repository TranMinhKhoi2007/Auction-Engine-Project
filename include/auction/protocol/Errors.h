#pragma once
#include <QJsonObject>
#include <QString>
#include <stdexcept>
namespace auction::protocol {
struct ErrorPayload {
    QString type;
    QString reason;
    QString message;
    QJsonObject data;
};
class ProtocolError : public std::runtime_error {
public:
    explicit ProtocolError(QString message, QString errorType = "BadRequest",QString errorReason={},QJsonObject errorData={})
        : std::runtime_error(message.toStdString()), type(std::move(errorType)),reason(std::move(errorReason)),data(std::move(errorData)) {}
    QString type,reason;
    QJsonObject data;
};
}
