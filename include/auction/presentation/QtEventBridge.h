#pragma once

#include "auction/domain/events/AuctionEvent.h"
#include "auction/client/IAuctionGateway.h"

#include <QObject>

namespace auction::presentation {

class QtEventBridge final : public QObject {
    Q_OBJECT

public:
    explicit QtEventBridge(client::IAuctionGateway &gateway,QObject *parent = nullptr);

signals:
    void eventReceived(const domain::AuctionEvent &event);
};

} // namespace auction::presentation
