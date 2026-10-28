#pragma once
#include "auction/protocol/Errors.h"
#include <functional>
#include <optional>
namespace auction::client {
using GatewayError=protocol::ErrorPayload;
template<class T> struct GatewayResult {
    std::optional<T> value;
    std::optional<GatewayError> error;
    qint64 serverNowMs{};
    QJsonObject metadata;
    explicit operator bool() const {return value.has_value() && !error;}
};
template<class T> using Callback=std::function<void(GatewayResult<T>)>;
}
