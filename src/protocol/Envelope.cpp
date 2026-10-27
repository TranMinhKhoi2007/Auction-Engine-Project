#include "auction/protocol/Envelope.h"
#include "auction/protocol/JsonCodec.h"
#include <cmath>
namespace auction::protocol {
QString encode(const QJsonObject &o) { return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)); }
Request parseRequest(const QString &message) {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) throw ProtocolError("JSON không hợp lệ");
    const auto o = document.object();
    if (!o.value("v").isDouble()) throw ProtocolError("Thiếu phiên bản");
    if (o.value("v").toDouble() != kVersion) throw ProtocolError("Phiên bản không tương thích", "ProtocolMismatch");
    return {readId(o.value("id")), readString(o.value("method")), readObject(o.value("params"))};
}
QJsonObject request(quint64 id, const QString &method, const QJsonObject &params) {
    return {{"v", kVersion}, {"id", static_cast<double>(id)}, {"method", method}, {"params", params}};
}
QJsonObject response(quint64 id, const QJsonValue &result, qint64 now) {
    return {{"v", kVersion}, {"id", static_cast<double>(id)}, {"ok", true}, {"result", result}, {"serverNowMs", now}};
}
QJsonObject failure(quint64 id, const ErrorPayload &error, qint64 now) {
    return {{"v", kVersion}, {"id", static_cast<double>(id)}, {"ok", false}, {"error", toJson(error)}, {"serverNowMs", now}};
}
}
