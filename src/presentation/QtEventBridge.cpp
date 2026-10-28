#include "auction/presentation/QtEventBridge.h"

namespace auction::presentation {

QtEventBridge::QtEventBridge(client::IAuctionGateway &gateway,QObject *parent) : QObject(parent) {
    connect(&gateway,&client::IAuctionGateway::eventReceived,this,&QtEventBridge::eventReceived);
}

} // namespace auction::presentation
