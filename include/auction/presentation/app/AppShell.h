#pragma once
#include "auction/presentation/QtLogSink.h"
#include "auction/presentation/app/HomePage.h"
#include "auction/presentation/app/WorkspacePage.h"
#include "auction/presentation/pages/AccountPages.h"
#include "auction/presentation/pages/AuctionDetailPage.h"
#include "auction/presentation/pages/InventoryPage.h"
#include "auction/presentation/pages/MarketplacePage.h"

#include <QMainWindow>
#include <memory>
namespace auction::presentation {
class AppShell : public QMainWindow {
    Q_OBJECT
public:
    AppShell(client::IAuctionGateway &gateway, std::shared_ptr<QtEventBridge> bridge,
             std::shared_ptr<QtLogSink> log, QWidget *parent = nullptr);
    void refreshAll() { store_.refreshAll(); }
    UserSession &session() { return session_; }
    AuctionStore &store() { return store_; }
    ThemeManager &themeManager() { return theme_; }
    void navigate(Page page, quint64 id = 0);
    void createUser(bool enterAfter = false);
    void createAuction();

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    void showPage(Page page, quint64 id);
    void topUp();
    void joinAuction(quint64 id);
    void placeBid(quint64 id, qint64 value);
    void cancelAuction(quint64 id);
    void approveAuction(quint64 id);
    void closeAuctionEarly(quint64 id);
    void deleteAuction(quint64 id);
    void resellItem(const application::AuctionDetailDto &item);
    void seedDemoAuctions();
    void followAuction(quint64 id);
    void processEvent(const domain::AuctionEvent &event);
    QString sessionNotice_;
    void notifyError(const QString &message);
    void status();
    bool canWrite(quint64 id=0) const;
    void updateConnection();
    void serverSettings();
    client::IAuctionGateway &gateway_;
    std::shared_ptr<QtEventBridge> bridge_;
    std::shared_ptr<QtLogSink> log_;
    ThemeManager theme_;
    UserSession session_;
    UiTicker ticker_;
    AuctionStore store_;
    ProductImageStore images_;
    UiContext context_;
    NavigationController navigation_;
    QStackedWidget *root_{};
    QWidget *disconnectedBanner_{};
    QLabel *connectionMessage_{};
    QLabel *connectionStatus_{},*loadingLabel_{};
    HomePage *home_{};
    WorkspacePage *workspace_{};
    MarketplacePage *marketplace_{};
    AuctionDetailPage *detail_{};
    MyAuctionsPage *mine_{};
    SellerDashboardPage *seller_{};
    InventoryPage *inventory_{};
    WalletPage *wallet_{};
    NotificationsPage *notifications_{};
    SystemLogPage *systemLog_{};
    ToastManager *toasts_{};
    QSet<QString> countdownNotified_;
    QSet<QString> closureNotified_;
    bool manualCollapse_{};
    bool hasConnected_{};
};
} // namespace auction::presentation
