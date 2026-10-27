#pragma once
#include "auction/protocol/Errors.h"
#include "auction/protocol/ProtocolConfig.h"
#include <QJsonDocument>
namespace auction::protocol {
struct Request { quint64 id; QString method; QJsonObject params; };
Request parseRequest(const QString &message);
QJsonObject request(quint64 id, const QString &method, const QJsonObject &params = {});
QJsonObject response(quint64 id, const QJsonValue &result, qint64 now);
QJsonObject failure(quint64 id, const ErrorPayload &error, qint64 now);
QString encode(const QJsonObject &object);
}
