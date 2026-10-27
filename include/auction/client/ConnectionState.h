#pragma once
#include <QMetaType>
namespace auction::client { enum class ConnectionState { Disconnected, Connecting, Reconnecting, Connected }; }
Q_DECLARE_METATYPE(auction::client::ConnectionState)
