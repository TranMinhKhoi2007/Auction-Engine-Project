#pragma once
#include "auction/presentation/state/AuctionStore.h"
#include "auction/presentation/state/ProductImageStore.h"
#include "auction/presentation/state/UiTicker.h"
#include "auction/presentation/theme/ThemeManager.h"
namespace auction::presentation {
struct UiContext {
    client::IAuctionGateway &gateway;
    AuctionStore &store;
    UserSession &session;
    UiTicker &ticker;
    ThemeManager &theme;
    ProductImageStore &images;
};
} // namespace auction::presentation
