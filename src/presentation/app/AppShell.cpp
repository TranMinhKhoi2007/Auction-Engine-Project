#include "auction/presentation/app/AppShell.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/dialogs/Dialogs.h"
#include "auction/presentation/util/UiHelpers.h"
#include "auction/protocol/JsonCodec.h"
#include "auction/media/ProductImage.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QShortcut>
#include <QStatusBar>
#include <QSettings>
#include <QUrl>
#include <QDir>
namespace auction::presentation {
AppShell::AppShell(client::IAuctionGateway &gateway, std::shared_ptr<QtEventBridge> bridge,
                   std::shared_ptr<QtLogSink> log, QWidget *parent)
    : QMainWindow(parent), gateway_(gateway), bridge_(std::move(bridge)), log_(std::move(log)),
      theme_(this), session_(this), ticker_(gateway.clock(), this),
      store_(gateway, *bridge_, session_, this), images_(this),
      context_{gateway, store_, session_, ticker_, theme_, images_}, navigation_(this) {
    setObjectName("appShell");
    images_.attachGateway(gateway_);
    connect(&store_,&AuctionStore::auctionChanged,this,[this](quint64 id) {
        if(const auto *auction=store_.get(id))images_.ensureImage(id,auction->hasImage);
    });
    connect(&images_,&ProductImageStore::loadFailed,this,[this](quint64 id,const QString &message) {
        log_->warn(("Không tải được ảnh phiên #"+QString::number(id)+": "+message).toStdString());
    });
    setWindowTitle("Auction Engine · Đấu giá thời gian thực");
    setMinimumSize(1180, 720);
    resize(1440, 860);
    root_ = new QStackedWidget(this);
    auto *container=new QWidget(this);
    auto *layout=new QVBoxLayout(container);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
    disconnectedBanner_=new QWidget(container);disconnectedBanner_->setObjectName("bannerDisconnected");
    auto *bannerLayout=new QHBoxLayout(disconnectedBanner_);
    connectionMessage_=label("Mất kết nối tới máy chủ. Đang thử lại…",disconnectedBanner_,14,"warning");connectionMessage_->setWordWrap(true);
    connectionMessage_->setObjectName("lblConnectionProblem");bannerLayout->addWidget(connectionMessage_,1);
    auto *retry=button("Thử lại ngay","btnRetryConnect",disconnectedBanner_);
    auto *settings=button("Đổi máy chủ…","btnChangeServer",disconnectedBanner_,"ghost");
    bannerLayout->addWidget(retry);bannerLayout->addWidget(settings);
    connect(retry,&QPushButton::clicked,&gateway_,&client::IAuctionGateway::retryNow);
    connect(settings,&QPushButton::clicked,this,&AppShell::serverSettings);
    layout->addWidget(disconnectedBanner_);
    loadingLabel_=label("Đang tải…",container,12,"muted");loadingLabel_->setObjectName("lblInitialLoading");
    layout->addWidget(loadingLabel_);layout->addWidget(root_,1);setCentralWidget(container);
    connectionStatus_=label("",this,12);connectionStatus_->setObjectName("lblConnectionStatus");
    connectionStatus_->setMinimumWidth(connectionStatus_->fontMetrics().horizontalAdvance("Đang kết nối lại…")+16);
    connectionStatus_->setWordWrap(false);statusBar()->addPermanentWidget(connectionStatus_);
    home_ = new HomePage(context_, root_);
    workspace_ = new WorkspacePage(context_, root_);
    root_->addWidget(home_);
    root_->addWidget(workspace_);
    marketplace_ = new MarketplacePage(context_, workspace_->pages);
    detail_ = new AuctionDetailPage(context_, workspace_->pages);
    mine_ = new MyAuctionsPage(context_, workspace_->pages);
    seller_ = new SellerDashboardPage(context_, workspace_->pages);
    inventory_ = new InventoryPage(context_, workspace_->pages);
    wallet_ = new WalletPage(context_, workspace_->pages);
    notifications_ = new NotificationsPage(context_, workspace_->pages);
    systemLog_ = new SystemLogPage(context_, workspace_->pages);
    for (QWidget *page : QList<QWidget *>{marketplace_, detail_, mine_, seller_, inventory_, wallet_,
                                          notifications_, systemLog_})
        workspace_->pages->addWidget(page);
    toasts_ = new ToastManager(ticker_, theme_, this);
    connect(&navigation_, &NavigationController::changed, this, &AppShell::showPage);
    connect(home_, &HomePage::enterRequested, this, [this] { navigate(Page::Marketplace); });
    connect(home_, &HomePage::openRequested, this,
            [this](quint64 id) { navigate(Page::Detail, id); });
    connect(home_, &HomePage::quickStartRequested, this, [this](int step) {
        if (step == 0 || !session_.loggedIn()) {
            createUser();
            return;
        }
        if (step == 1) topUp();
        else if (step == 2) createAuction();
        else navigate(step == 3 ? Page::Seller : Page::Marketplace);
    });
    connect(workspace_->sidebar, &NavSidebar::navigate, this,
            [this](Page page) { navigate(page); });
    connect(workspace_->sidebar, &NavSidebar::toggleRequested, this, [this] {
        manualCollapse_ = !workspace_->sidebar->collapsed();
        workspace_->sidebar->setCollapsed(manualCollapse_);
    });
    connect(workspace_->topbar, &TopBar::createUserRequested, this, [this] {
        if(!session_.loggedIn()){createUser();return;}
        const auto page=navigation_.page();const auto id=navigation_.auctionId();
        gateway_.logout(this,[this,page,id](auto result){
            if(!result){notifyError(result.error->message);return;}
            createUser();if(auto *dialog=findChild<QDialog *>("loginDialog"))connect(dialog,&QDialog::accepted,this,[this,page,id]{navigate(page,id);});
        });
    });
    connect(workspace_->topbar, &TopBar::logoutRequested, this, [this] {
        gateway_.logout(this,[this](auto r) {
            if(!r){notifyError(r.error->message);return;}
            session_.setUser(std::nullopt);navigation_.go(Page::Home);
        });
    });
    connect(workspace_->topbar, &TopBar::topUpRequested, this, &AppShell::topUp);
    connect(workspace_->topbar, &TopBar::openRequested, this,
            [this](quint64 id) { navigate(Page::Detail, id); });
    connect(workspace_->topbar->search, &QLineEdit::textEdited, this, [this](const QString &text) {
        marketplace_->setSearch(text);
        if (navigation_.page() != Page::Marketplace)
            navigate(Page::Marketplace);
    });
    for (auto *page : QList<MarketplacePage *>{marketplace_, mine_, seller_}) {
        connect(page, &MarketplacePage::openRequested, this,
                [this](quint64 id) { navigate(Page::Detail, id); });
        connect(page, &MarketplacePage::createRequested, this, &AppShell::createAuction);
        connect(page, &MarketplacePage::createUserRequested, this, [this] { createUser(); });
        connect(page, &MarketplacePage::approveRequested, this, &AppShell::approveAuction);
        connect(page, &MarketplacePage::cancelRequested, this, &AppShell::cancelAuction);
        connect(page, &MarketplacePage::seedDemoRequested, this, &AppShell::seedDemoAuctions);
    }
    connect(detail_, &AuctionDetailPage::backRequested, this, [this] { navigation_.back(); });
    connect(detail_, &AuctionDetailPage::joinRequested, this, &AppShell::joinAuction);
    connect(detail_, &AuctionDetailPage::bidRequested, this, &AppShell::placeBid);
    connect(detail_, &AuctionDetailPage::cancelRequested, this, &AppShell::cancelAuction);
    connect(detail_, &AuctionDetailPage::closeEarlyRequested, this, &AppShell::closeAuctionEarly);
    connect(detail_, &AuctionDetailPage::approveRequested, this, &AppShell::approveAuction);
    connect(detail_, &AuctionDetailPage::deleteRequested, this, &AppShell::deleteAuction);
    connect(detail_, &AuctionDetailPage::followRequested, this, &AppShell::followAuction);
    connect(detail_, &AuctionDetailPage::topUpRequested, this, &AppShell::topUp);
    connect(wallet_, &WalletPage::topUpRequested, this, &AppShell::topUp);
    connect(wallet_, &WalletPage::openRequested, this,
            [this](quint64 id) { navigate(Page::Detail, id); });
    connect(notifications_, &NotificationsPage::openRequested, this,
            [this](quint64 id) { navigate(Page::Detail, id); });
    connect(notifications_, &NotificationsPage::unreadChanged, this, [this](int count) {
        workspace_->topbar->notificationBadge->setText(QString::number(count));
    });
    connect(inventory_, &InventoryPage::openAuctionRequested, this,
            [this](quint64 id) { navigate(Page::Detail, id); });
    connect(inventory_, &InventoryPage::exploreMarketplaceRequested, this,
            [this] { navigate(Page::Marketplace); });
    connect(inventory_, &InventoryPage::resellRequested, this,
            [this](const application::AuctionDetailDto &item) { resellItem(item); });
    connect(workspace_->topbar, &TopBar::inventoryRequested, this,
            [this] { navigate(Page::Inventory); });
    connect(workspace_->topbar, &TopBar::changePasswordRequested, this, [this] {
        if (!session_.loggedIn()) { createUser(); return; }
        auto *dialog = new ChangePasswordDialog(context_, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &ChangePasswordDialog::passwordChanged, this, [this] {
            toasts_->showToast("Đã đổi mật khẩu thành công! Mật khẩu mới đã được lưu vào hệ thống.", "success");
        });
        dialog->open();
    });
    connect(workspace_->topbar->bell, &QPushButton::clicked, this, [this] {
        auto *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        auto latest = notifications_->latest();
        if (latest.isEmpty()) {
            auto *none = menu->addAction("Chưa có thông báo");
            none->setEnabled(false);
        }
        for (int i = 0; i < latest.size(); ++i) {
            auto full = latest[i];
            auto text = full.split('\n').last();
            auto *a = menu->addAction(text.left(85));
            a->setToolTip(full);
            connect(a, &QAction::triggered, this, [this, i] { notifications_->openLatest(i); });
        }
        menu->addSeparator();
        connect(menu->addAction("Xem tất cả thông báo"), &QAction::triggered, this,
                [this] { navigate(Page::Notifications); });
        menu->popup(
            workspace_->topbar->bell->mapToGlobal(QPoint(0, workspace_->topbar->bell->height())));
    });
    connect(log_.get(), &QtLogSink::logReceived, systemLog_, &SystemLogPage::appendLog);
    connect(&store_, &AuctionStore::eventProcessed, this, &AppShell::processEvent);
    connect(&session_, &UserSession::changed, this, [this] {
        status();
    });
    connect(&store_, &AuctionStore::auctionChanged, this, [this] { status(); });
    auto shortcut = [this](const QString &key, std::function<void()> fn) {
        auto *s = new QShortcut(QKeySequence(key), this);
        connect(s, &QShortcut::activated, this, std::move(fn));
    };
    shortcut("Ctrl+K", [this] {
        navigate(Page::Marketplace);
        workspace_->topbar->search->setFocus();
        workspace_->topbar->search->selectAll();
    });
    shortcut("Ctrl+B", [this] {
        manualCollapse_ = !workspace_->sidebar->collapsed();
        workspace_->sidebar->setCollapsed(manualCollapse_);
    });
    shortcut("Ctrl+N", [this] { createAuction(); });
    const QList<Page> shortcuts = {Page::Marketplace, Page::MyAuctions, Page::Seller,
                                   Page::Inventory,   Page::Wallet,     Page::Notifications,
                                   Page::Log};
    for (int i = 0; i < shortcuts.size(); ++i)
        shortcut("Alt+" + QString::number(i + 1), [this, page = shortcuts[i]] { navigate(page); });
    hasConnected_=gateway_.state()==client::ConnectionState::Connected;
    connect(&gateway_,&client::IAuctionGateway::connectionStateChanged,this,[this](auto state) {
        if(state==client::ConnectionState::Connected)connectionMessage_->setText("Mất kết nối tới máy chủ. Đang thử lại…");
        updateConnection();
        if(state==client::ConnectionState::Connected) {
            toasts_->showToast(hasConnected_?"Đã kết nối lại":"Đã kết nối","success");
            hasConnected_=true;
            if(!sessionNotice_.isEmpty() && !session_.loggedIn())QTimer::singleShot(0,this,[this]{createUser();});
        } else if(state==client::ConnectionState::Disconnected && hasConnected_) {
            toasts_->showToast("Mất kết nối","warning");
        }
    });
    connect(&gateway_,&client::IAuctionGateway::sessionEnded,this,[this](const QString &reason) {
        for(auto *dialog:findChildren<QDialog *>("loginDialog")) {
            dialog->setObjectName("expiredLoginDialog");dialog->reject();
        }
        session_.setUser(std::nullopt);navigation_.go(Page::Home);
        if(reason=="LoggedOut" || reason=="ServerChanged")return;
        sessionNotice_=reason=="Replaced"?"Tài khoản đã đăng nhập ở nơi khác":"Phiên đăng nhập đã hết hạn, vui lòng đăng nhập lại";
        if(gateway_.state()==client::ConnectionState::Connected)createUser();
    });
    connect(&gateway_,&client::IAuctionGateway::forbidden,this,[this]{toasts_->showToast("Bạn không có quyền thực hiện thao tác này","danger");});
    connect(&gateway_,&client::IAuctionGateway::clientLog,log_.get(),&QtLogSink::logReceived);
    connect(&gateway_,&client::IAuctionGateway::connectionProblem,this,[this](const QString &message){connectionMessage_->setText(message);disconnectedBanner_->show();});
    connect(&store_,&AuctionStore::loadingChanged,this,[this]{loadingLabel_->setVisible(!store_.loaded() && gateway_.state()==client::ConnectionState::Connected);emit store_.auctionListChanged();});
    updateConnection();store_.refreshAll();
    root_->setCurrentIndex(0);
    status();
}
void AppShell::navigate(Page page, quint64 id) {
    if (page != Page::Home && !session_.loggedIn()) {
        createUser(true);
        return;
    }
    navigation_.go(page, id);
}
void AppShell::showPage(Page page, quint64 id) {
    if (page == Page::Home || !session_.loggedIn()) {
        root_->setCurrentWidget(home_);
        return;
    }
    root_->setCurrentWidget(workspace_);
    workspace_->pages->setCurrentIndex(static_cast<int>(page));
    workspace_->sidebar->select(page);
    const QStringList titles = {"Sàn đấu giá", "Chi tiết phiên", "Phiên của tôi", "Bán hàng",
                                "Kho lưu trữ", "Ví của tôi",     "Thông báo",     "Nhật ký"};
    workspace_->topbar->setTitle(titles[static_cast<int>(page)]);
    if (page == Page::Detail)
        detail_->openAuction(id);
    auto *widget = workspace_->pages->currentWidget();
    auto *effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    auto *animation = new QPropertyAnimation(effect, "opacity", widget);
    animation->setDuration(150);
    animation->setStartValue(0.5);
    animation->setEndValue(1.0);
    connect(animation, &QPropertyAnimation::finished, widget,
            [widget] { widget->setGraphicsEffect(nullptr); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
void AppShell::notifyError(const QString &text) {
    log_->error(text.toStdString());
    toasts_->showToast(text, "danger");
}
void AppShell::createUser(bool enterAfter) {
    if(session_.loggedIn()) {gateway_.logout(this,[this,enterAfter](auto result){if(!result){notifyError(result.error->message);return;}createUser(enterAfter);});return;}
    for(auto *existing:findChildren<QDialog *>("loginDialog"))if(existing->isVisible()){existing->raise();existing->activateWindow();return;}
    auto *dialog = new LoginDialog(context_, this);
    dialog->setNotice(sessionNotice_);sessionNotice_.clear();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, [this, dialog, enterAfter] {
        session_.setUser(dialog->authenticatedUser());
        store_.refreshPeople();
        if (enterAfter)
            navigate(Page::Marketplace);
        toasts_->showToast("Đã đăng nhập.", "success");
        // A user who was signed out still receives their settlement notification.
        gateway_.getInbox(this,[this](auto r) {
            if(r)for(const auto &event:*r.value)if(std::holds_alternative<domain::AuctionClosedEvent>(event))processEvent(event);
        });
    });
    dialog->open();
}
bool AppShell::canWrite(quint64 id) const {
    return gateway_.state()==client::ConnectionState::Connected && session_.loggedIn() && !store_.busy(id);
}
void AppShell::topUp() {
    if(!session_.user){createUser();return;}if(!canWrite()){return;}
    auto *dialog=new TopUpDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog,&QDialog::accepted,this,[this,dialog] {
        if(!canWrite()){return;}store_.setBusy(0,true);
        gateway_.topUp(dialog->amount(),this,[this](auto r) {
            store_.setBusy(0,false);if(!r){notifyError(r.error->message);return;}
            store_.refreshPeople();toasts_->showToast("Đã nạp tiền vào ví.","success");
        });
    });dialog->open();
}
void AppShell::createAuction() {
    if(!session_.loggedIn()){createUser();return;}if(!canWrite()){return;}
    auto *dialog=new CreateAuctionWizard(context_,this);dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog,&CreateAuctionWizard::createRequested,this,[this,dialog] {
        if(!canWrite()){return;}
        auto command=dialog->command();
        if(!dialog->imagePath().isEmpty()) {
            try {command.imageData=media::importProductImage(dialog->imagePath()).toStdString();}
            catch(const std::exception &error) {dialog->showError(QString::fromUtf8(error.what()));return;}
        }
        store_.setBusy(0,true);dialog->setEnabled(false);
        QPointer<CreateAuctionWizard> guard(dialog);
        gateway_.createAuction(std::move(command),this,[this,guard](auto r) {
            store_.setBusy(0,false);if(guard)guard->setEnabled(true);
            if(!r){if(guard)guard->showError(r.error->message);log_->error(r.error->message.toStdString());return;}
            const auto id=*r.value;
            store_.refreshAuction(id.value());if(guard)guard->accept();navigate(Page::Detail,id.value());
            toasts_->showToast("Phiên đang chờ duyệt, hãy vào Bán hàng để duyệt","success");
        });
    });dialog->open();
}
void AppShell::joinAuction(quint64 id) {
    const auto *a=store_.get(id);
    if(!a || !canWrite(id) || a->sellerId==session_.id() || isClosed(*a) || a->state=="Chờ duyệt") {
        detail_->setError("Phiên chưa mở hoặc đã đóng, không thể tham gia.");return;
    }
    if(hasJoined(*a,session_.id())){return;}auto wallet=store_.wallet();if(!wallet){return;}
    if(wallet->available<a->depositAmount){detail_->setError("Số dư khả dụng không đủ để đặt cọc "+Formatters::formatMoney(a->depositAmount),"Nạp tiền");return;}
    auto *dialog=new ConfirmJoinDialog(*a,*wallet,this);dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog,&QDialog::accepted,this,[this,id] {
        if(!canWrite(id)){return;}store_.setBusy(id,true);
        gateway_.joinAuction(common::AuctionId{id},this,[this,id](auto r) {
            store_.setBusy(id,false);
            if(!r){detail_->setError(r.error->message,"Nạp tiền");log_->warn(r.error->message.toStdString());return;}
            session_.setFollowed(id,true);store_.refreshAuction(id);store_.refreshPersonal();detail_->setError({});
            toasts_->showToast("Tham gia thành công · Đã giữ tiền ký quỹ.","success");
        });
    });dialog->open();
}
void AppShell::placeBid(quint64 id,qint64 value) {
    if(!canWrite(id)){return;}store_.setBusy(id,true);
    gateway_.placeBid(common::AuctionId{id},domain::Money(value),this,[this,id,value](auto r) {
        store_.setBusy(id,false);
        if(r){store_.refreshAuction(id);detail_->setError({});toasts_->showToast("Đặt giá thành công: "+Formatters::formatMoney(domain::Money(value)),"success");return;}
        const auto &e=*r.error;QString text=e.message,action;
        if(e.type=="InvalidBid") {
            text=Formatters::formatBidRejectReason(e.reason);
            if(e.reason=="BelowStartingPrice" || e.reason=="BelowMinimumIncrement") {
                if(auto *a=store_.get(id)) {
                    const auto minimum=e.data.contains("minimumNextBid")?protocol::readMoney(e.data.value("minimumNextBid")):a->minimumNextBid;
                    text="Giá đặt phải từ "+Formatters::formatMoney(minimum)+" trở lên (bước giá "+Formatters::formatMoney(a->minIncrement)+").";
                }
                action="Điền giá tối thiểu";
            } else if(e.reason=="InsufficientFunds")action="Nạp tiền";
            else if(e.reason=="DepositNotPaid")action="Tham gia";
        }
        detail_->setError(text,action);log_->warn(e.message.toStdString());store_.refreshAuction(id);
    });
}
void AppShell::approveAuction(quint64 id) {
    const auto *a=store_.get(id);if(!canWrite(id)||!a||a->sellerId!=session_.id()||a->state!="Chờ duyệt"){return;}
    store_.setBusy(id,true);gateway_.approveAuction(common::AuctionId{id},this,[this,id](auto r) {
        store_.setBusy(id,false);if(!r){notifyError(r.error->message);return;}
        store_.refreshAuction(id);toasts_->showToast("Đã duyệt phiên đấu giá.","success");
    });
}
void AppShell::closeAuctionEarly(quint64 id) {
    const auto *a=store_.get(id);
    if(!a||!canWrite(id)||a->sellerId!=session_.id()||!isLive(*a)||a->bidHistory.empty()){return;}
    store_.setBusy(id,true);
    gateway_.getAuctionDetail(common::AuctionId{id},this,[this,id](auto preview) {
        store_.setBusy(id,false);
        if(!preview){notifyError(preview.error->message);return;}
        if(!canWrite(id)||preview.value->sellerId!=session_.id()||!isLive(*preview.value)){return;}
        auto *dialog=new ConfirmCloseEarlyDialog(*preview.value,preview.metadata.value("leaderCanSettle").toBool(),this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog,&QDialog::accepted,this,[this,id] {
            if(!canWrite(id)){return;}store_.setBusy(id,true);
            gateway_.closeAuctionEarly(common::AuctionId{id},this,[this,id](auto r) {
                store_.setBusy(id,false);if(!r){notifyError(r.error->message);return;}
                store_.refreshAuction(id);store_.refreshPeople();detail_->setError({});
            });
        });dialog->open();
    });
}
void AppShell::cancelAuction(quint64 id) {
    const auto *a=store_.get(id);if(!a||!canWrite(id)||isClosed(*a)||a->sellerId!=session_.id()){return;}
    auto *dialog=new ConfirmCancelDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog,&QDialog::accepted,this,[this,id,dialog] {
        if(!canWrite(id)){return;}store_.setBusy(id,true);
        gateway_.cancelAuction(common::AuctionId{id},dialog->reason().toStdString(),this,[this,id](auto r) {
            store_.setBusy(id,false);if(!r){notifyError(r.error->message);return;}
            store_.refreshAuction(id);store_.refreshPersonal();toasts_->showToast("Đã hủy phiên và hoàn cọc cho người tham gia.","warning");
        });
    });dialog->open();
}
void AppShell::deleteAuction(quint64 id) {
    if (!canWrite(id)) return;
    store_.setBusy(id, true);
    gateway_.deleteAuction(common::AuctionId{id}, this, [this, id](auto r) {
        store_.setBusy(id, false);
        if (!r) {
            notifyError(r.error->message);
            return;
        }
        toasts_->showToast("Đã xóa phiên đấu giá thành công.", "success");
        store_.refreshAll();
        navigate(Page::Marketplace);
    });
}
void AppShell::resellItem(const application::AuctionDetailDto &item) {
    if (!session_.loggedIn()) { createUser(); return; }
    if (!canWrite()) { return; }
    auto *dialog = new CreateAuctionWizard(context_, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    domain::Money suggestedPrice = item.currentPrice.amount() > 0 ? item.currentPrice : item.startingPrice;
    dialog->prefill(QString::fromStdString(item.productName),
                    QString::fromStdString(item.category),
                    QString::fromStdString(item.productDescription),
                    suggestedPrice);

    const auto autoAttachImage = [this, dialog, itemId = item.id.value()]() -> bool {
        if (images_.hasLoadedImage(itemId)) {
            const QImage raw = images_.rawImage(itemId);
            if (!raw.isNull()) {
                const QString tempFile = QDir::tempPath() + QString("/resell_%1.jpg").arg(itemId);
                if (raw.save(tempFile, "JPG", 92)) {
                    dialog->setImagePath(tempFile);
                    return true;
                }
            }
        }
        return false;
    };

    if (!autoAttachImage()) {
        if (item.hasImage) {
            images_.ensureImage(item.id.value(), true);
            auto conn = std::make_shared<QMetaObject::Connection>();
            *conn = connect(&images_, &ProductImageStore::changed, dialog,
                            [dialog, autoAttachImage, itemId = item.id.value(), conn](quint64 changedId) {
                if (changedId == itemId) {
                    if (dialog->imagePath().isEmpty()) {
                        autoAttachImage();
                    }
                    QObject::disconnect(*conn);
                }
            });
        }

        if (dialog->imagePath().isEmpty()) {
            const QString name = QString::fromStdString(item.productName).toLower();
            QString fallbackRes;
            if (name.contains("bình gốm hoa lam") || name.contains("gốm thời minh")) fallbackRes = ":/demo/vase.jpg";
            else if (name.contains("phố cổ hà nội") || name.contains("bùi xuân phái")) fallbackRes = ":/demo/lacquer.jpg";
            else if (name.contains("macbook")) fallbackRes = ":/demo/macbook.jpg";
            else if (name.contains("iphone")) fallbackRes = ":/demo/iphone.jpg";
            else if (name.contains("rolex")) fallbackRes = ":/demo/rolex.jpg";
            else if (name.contains("thiếu nữ bên hoa huệ") || name.contains("tô ngọc vân")) fallbackRes = ":/demo/art_silk.jpg";
            else if (name.contains("tượng phật")) fallbackRes = ":/demo/statue.jpg";
            else if (name.contains("leica")) fallbackRes = ":/demo/leica.jpg";
            else if (name.contains("bình vôi")) fallbackRes = ":/demo/lime_pot.jpg";
            else if (name.contains("kết nạp đảng") || name.contains("nguyễn sáng")) fallbackRes = ":/demo/art_lacquer.jpg";
            else if (name.contains("chợ nổi") || name.contains("nguyễn gia trí")) fallbackRes = ":/demo/art_floating.jpg";
            else if (name.contains("máy chủ") || name.contains("server") || name.contains("dell poweredge")) fallbackRes = ":/demo/server.jpg";

            if (!fallbackRes.isEmpty()) {
                dialog->setImagePath(fallbackRes);
            }
        }
    }

    connect(dialog, &CreateAuctionWizard::createRequested, this, [this, dialog] {
        if (!canWrite()) { return; }
        auto command = dialog->command();
        if (!dialog->imagePath().isEmpty()) {
            try { command.imageData = media::importProductImage(dialog->imagePath()).toStdString(); }
            catch (const std::exception &error) { dialog->showError(QString::fromUtf8(error.what())); return; }
        }
        store_.setBusy(0, true);
        dialog->setEnabled(false);
        QPointer<CreateAuctionWizard> guard(dialog);
        gateway_.createAuction(std::move(command), this, [this, guard](auto r) {
            store_.setBusy(0, false);
            if (guard) guard->setEnabled(true);
            if (!r) {
                if (guard) guard->showError(r.error->message);
                log_->error(r.error->message.toStdString());
                return;
            }
            const auto id = *r.value;
            store_.refreshAuction(id.value());
            if (guard) guard->accept();
            navigate(Page::Detail, id.value());
            toasts_->showToast("Đã đăng bán lại vật phẩm thành công! Phiên đang ở trạng thái Chờ duyệt.", "success");
        });
    });
    dialog->open();
}
void AppShell::seedDemoAuctions() {
    if (!session_.loggedIn()) {
        createUser();
        return;
    }
    if (!canWrite()) {
        return;
    }

    struct Preset {
        application::ProductType type;
        QString name;
        QString desc;
        qint64 starting;
        qint64 increment;
        qint64 deposit;
        int delay;
        int duration;
        int century;
        QString origin;
        QString preservation;
        bool appraised;
        QString artist;
        int year;
        QString medium;
        QString dims;
        bool cert;
        QString brand;
        QString model;
        int warranty;
        bool isNew;
        QString imageResource;
    };

    const std::vector<Preset> presets = {
        {
            application::ProductType::DoCo,
            "Bình gốm hoa lam thời Minh (Thế kỷ 15)",
            "Cổ vật gốm sứ hoa lam men ngọc thời Minh tuyệt hảo, hoa văn rồng cuộn tinh xảo nguyên bản, có chứng thư giám định bảo tàng.",
            50000000, 2000000, 5000000, 10, 300,
            15, "Cảnh Đức Trấn, Trung Hoa", "Nguyên vẹn 98%", true,
            "", 0, "", "", false, "", "", 0, false,
            ":/demo/vase.jpg"
        },
        {
            application::ProductType::TranhNgheThuat,
            "Tranh sơn mài Phố Cổ Hà Nội - Danh họa Bùi Xuân Phái",
            "Kiệt tác sơn mài khắc họa không gian cổ kính Hà thành mùa thu 1972, có chữ ký tác giả góc phải và giấy chứng nhận xác thực hội đồng mỹ thuật.",
            120000000, 5000000, 10000000, 15, 360,
            0, "", "", false,
            "Bùi Xuân Phái", 1972, "Sơn mài trên gỗ dổi", "80cm x 120cm", true,
            "", "", 0, false,
            ":/demo/art.jpg"
        },
        {
            application::ProductType::ThietBiCongNghe,
            "MacBook Pro 16 inch M3 Max 64GB / 1TB SSD Space Black",
            "Bản max cấu hình M3 Max nguyên seal Fullbox, bảo hành chính hãng Apple Care+ đến 2026, xuất hóa đơn VAT đầy đủ.",
            65000000, 1000000, 5000000, 10, 240,
            0, "", "", false,
            "", 0, "", "", false,
            "Apple", "MacBook Pro 16 M3 Max", 24, true,
            ":/demo/macbook.jpg"
        },
        {
            application::ProductType::DoCo,
            "Trống đồng Đông Sơn loại 1 (Thời kỳ Văn Lang)",
            "Báu vật khảo cổ niên đại 2500 năm, hoa văn ngôi sao 14 cánh và chim Lạc sắc nét, bảo vật quốc gia phiên bản đấu giá độc quyền.",
            250000000, 10000000, 25000000, 15, 420,
            5, "Đông Sơn, Thanh Hóa", "Gỉ đồng phong hóa tự nhiên tuyệt đẹp", true,
            "", 0, "", "", false, "", "", 0, false,
            ":/demo/drum.jpg"
        },
        {
            application::ProductType::TranhNgheThuat,
            "Tranh lụa Thiếu nữ bên hoa huệ - Danh họa Tô Ngọc Vân",
            "Bản phục chế thủ công cao cấp trên lụa tơ tằm Hà Đông nguyên bản, nét vẽ thanh tao đài các đặc trưng phong cách Mỹ thuật Đông Dương.",
            85000000, 3000000, 8000000, 10, 300,
            0, "", "", false,
            "Tô Ngọc Vân", 1943, "Màu nước trên lụa", "60cm x 90cm", true,
            "", "", 0, false,
            ":/demo/art_silk.jpg"
        },
        {
            application::ProductType::ThietBiCongNghe,
            "iPhone 16 Pro Max 1TB Titan Tự Nhiên (VN/A)",
            "Siêu phẩm Apple vỏ titan chuẩn hàng không vũ trụ, chip A18 Pro, nguyên hộp seal chưa kích hoạt, bảo hành 12 tháng chính hãng.",
            42000000, 1000000, 4000000, 10, 240,
            0, "", "", false,
            "", 0, "", "", false,
            "Apple", "iPhone 16 Pro Max", 12, true,
            ":/demo/iphone.jpg"
        },
        {
            application::ProductType::DoCo,
            "Bình vôi gốm men rạn thời Lý (Thế kỷ 12)",
            "Cổ vật gốm men rạn hoa nâu đặc trưng triều Lý, quai đắp nổi hình rồng cuộn, men bóng tự nhiên nguyên vẹn không sứt mẻ.",
            38000000, 1500000, 3500000, 10, 300,
            12, "Thăng Long, Đại Việt", "Nguyên vẹn 95%", true,
            "", 0, "", "", false, "", "", 0, false,
            ":/demo/lime_pot.jpg"
        },
        {
            application::ProductType::ThietBiCongNghe,
            "Máy ảnh Leica M11-P Rangefinder 60MP Safari Edition",
            "Phiên bản máy ảnh cơ quang học thủ công sản xuất giới hạn tại Đức, tích hợp công nghệ ký số xác thực ảnh Content Credentials.",
            220000000, 5000000, 20000000, 15, 360,
            0, "", "", false,
            "", 0, "", "", false,
            "Leica", "M11-P Safari", 36, true,
            ":/demo/leica.jpg"
        },
        {
            application::ProductType::TranhNgheThuat,
            "Tranh sơn dầu Chợ nổi Cái Răng - Họa sĩ Nguyễn Trung",
            "Bức họa khổ lớn miêu tả nhịp sống sông nước miền Tây rực rỡ sắc màu ban mai, từng đạt giải thưởng Triển lãm Mỹ thuật Toàn quốc.",
            55000000, 2000000, 5000000, 10, 300,
            0, "", "", false,
            "Nguyễn Trung", 2018, "Sơn dầu trên toan", "100cm x 150cm", true,
            "", "", 0, false,
            ":/demo/art_floating.jpg"
        },
        {
            application::ProductType::DoCo,
            "Tượng Phật Thích Ca mạ vàng thời Hậu Lê (Thế kỷ 17)",
            "Tượng đồng đúc thủ công mạ vàng lá cổ, gương mặt từ bi trang nghiêm, áo cà sa xếp nếp tinh xảo, báu vật tâm linh quý hiếm.",
            95000000, 5000000, 9000000, 15, 360,
            17, "Kinh Bắc, Đại Việt", "Lớp mạ vàng nguyên gốc", true,
            "", 0, "", "", false, "", "", 0, false,
            ":/demo/statue.jpg"
        },
        {
            application::ProductType::ThietBiCongNghe,
            "Đồng hồ Rolex Submariner Date 41mm Vàng Trắng",
            "Mẫu thợ lặn biểu tượng thế giới, mặt số đen và vành gốm Cerachrom xanh, máy tự động Calibre 3235 trữ cót 70 giờ, Fullbox sổ thẻ.",
            380000000, 10000000, 30000000, 15, 420,
            0, "", "", false,
            "", 0, "", "", false,
            "Rolex", "Submariner Date 126619LB", 60, true,
            ":/demo/rolex.jpg"
        },
        {
            application::ProductType::TranhNgheThuat,
            "Tranh sơn mài khắc bạc Rừng Cọ Đồi Chè - Nguyễn Gia Trí",
            "Tác phẩm sơn mài truyền thống kết hợp kỹ thuật thếp vàng bạc điêu luyện, tái hiện vẻ đẹp hùng vĩ của vùng trung du Bắc Bộ.",
            195000000, 5000000, 15000000, 15, 360,
            0, "", "", false,
            "Nguyễn Gia Trí", 1965, "Sơn mài khắc bạc thếp vàng", "90cm x 160cm", true,
            "", "", 0, false,
            ":/demo/art_lacquer.jpg"
        }
    };

    QSet<QString> existingNames;
    for (const auto &a : store_.auctions()) {
        existingNames.insert(QString::fromStdString(a.productName));
    }

    std::vector<Preset> toAdd;
    for (const auto &p : presets) {
        if (!existingNames.contains(p.name)) {
            toAdd.push_back(p);
            if (toAdd.size() >= 4) break;
        }
    }

    if (toAdd.empty()) {
        toasts_->showToast("Tất cả các phiên mẫu phong phú đã có sẵn trên sàn đấu giá!", "info");
        return;
    }

    auto total = static_cast<int>(toAdd.size());
    auto completed = std::make_shared<int>(0);
    auto successCount = std::make_shared<int>(0);

    for (const auto &p : toAdd) {
        application::CreateAuctionCommand cmd;
        cmd.sellerId = session_.id();
        cmd.productCommand.sellerId = session_.id();
        cmd.productCommand.type = p.type;
        cmd.productCommand.name = p.name.toStdString();
        cmd.productCommand.description = p.desc.toStdString();
        cmd.productCommand.eraCentury = p.century;
        cmd.productCommand.origin = p.origin.toStdString();
        cmd.productCommand.preservationState = p.preservation.toStdString();
        cmd.productCommand.isAppraised = p.appraised;
        cmd.productCommand.artist = p.artist.toStdString();
        cmd.productCommand.yearCreated = p.year;
        cmd.productCommand.medium = p.medium.toStdString();
        cmd.productCommand.dimensions = p.dims.toStdString();
        cmd.productCommand.hasCertificate = p.cert;
        cmd.productCommand.brand = p.brand.toStdString();
        cmd.productCommand.model = p.model.toStdString();
        cmd.productCommand.warrantyMonths = p.warranty;
        cmd.productCommand.isNew = p.isNew;
        cmd.startingPrice = domain::Money(p.starting);
        cmd.minIncrement = domain::Money(p.increment);
        cmd.depositAmount = domain::Money(p.deposit);
        cmd.startDelay = common::Seconds(p.delay);
        cmd.duration = common::Seconds(p.duration);
        if (!p.imageResource.isEmpty()) {
            try {
                cmd.imageData = media::importProductImage(p.imageResource).toStdString();
            } catch (const std::exception &e) {
                log_->warn(std::string("Không thể nạp ảnh mẫu: ") + e.what());
            }
        }

        gateway_.createAuction(std::move(cmd), this, [this, total, completed, successCount](auto r) {
            (*completed)++;
            if (r) {
                (*successCount)++;
                store_.refreshAuction(r.value->value());
            }
            if (*completed == total) {
                store_.refreshAll();
                toasts_->showToast(
                    QString("Đã nạp thành công %1 phiên mẫu mới! Các phiên đang ở trạng thái 'Chờ duyệt'.").arg(*successCount),
                    "success");
            }
        });
    }
}

void AppShell::followAuction(quint64 id) {
    const auto *a=store_.get(id);if(!a||!canWrite(id)||a->sellerId==session_.id()||isClosed(*a)){return;}
    const bool on=!session_.follows(id);store_.setBusy(id,true);
    const client::Callback<QJsonObject> callback=[this,id,on](auto r){store_.setBusy(id,false);if(!r){notifyError(r.error->message);return;}session_.setFollowed(id,on);};
    if(on)gateway_.followAuction(common::AuctionId{id},this,callback);else gateway_.unfollowAuction(common::AuctionId{id},this,callback);
}
void AppShell::updateConnection() {
    const auto state=gateway_.state();const bool connected=state==client::ConnectionState::Connected;
    connectionStatus_->setText(connected?"Đã kết nối":state==client::ConnectionState::Disconnected?"Mất kết nối":"Đang kết nối lại…");
    tone(connectionStatus_,connected?"success":state==client::ConnectionState::Disconnected?"danger":"warning");
    disconnectedBanner_->setVisible(!connected);loadingLabel_->setVisible(connected&&!store_.loaded());
    emit store_.auctionListChanged();emit store_.personalChanged();
    const auto ids=store_.auctions().keys();for(auto id:ids)emit store_.auctionChanged(id);
    for(auto *dialog:findChildren<QDialog *>())for(auto *b:dialog->findChildren<QPushButton *>()) {
        if(b->objectName()=="topUpSubmit" || b->objectName()=="confirmJoinSubmit" || b->objectName()=="confirmCloseEarlySubmit" || b->objectName()=="confirmCancelSubmit") {
            b->setEnabled(connected);b->setToolTip(connected?QString():"Mất kết nối tới máy chủ");
        }
    }
}
void AppShell::serverSettings() {
    auto *dialog=new ServerSettingsDialog(gateway_.serverUrl(),this);dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog,&QDialog::accepted,this,[this,dialog]{gateway_.setCaCertificateFile(dialog->caCertificateFile());gateway_.connectToServer(dialog->url());});dialog->open();
}
void AppShell::processEvent(const domain::AuctionEvent &event) {
    systemLog_->appendLog("INFO", store_.eventText(event));
    std::visit(
        [this](const auto &e) {
            using E = std::decay_t<decltype(e)>;
            const auto *a = store_.get(e.auctionId.value());
            if (!a)
                return;
            const auto title = QString::fromStdString(a->productName);
            const auto open = [this, id = e.auctionId.value()] { navigate(Page::Detail, id); };
            const bool joined = hasJoined(*a, session_.id());
            if constexpr (std::is_same_v<E, domain::OutbidEvent>) {
                if (e.previousLeader == session_.id())
                    toasts_->showToast("Bạn đã bị vượt giá ở «" + title +
                                           "»: " + Formatters::formatMoney(e.newAmount),
                                       "danger", "Đặt giá lại", open);
            } else if constexpr (std::is_same_v<E, domain::AuctionClosedEvent>) {
                const auto key = QString::number(session_.id().value()) + "/" +
                                 QString::number(e.auctionId.value());
                if (closureNotified_.contains(key))
                    return;
                if (joined || a->sellerId == session_.id())
                    closureNotified_.insert(key);
                if (e.closedEarly && a->sellerId == session_.id()) {
                    toasts_->showToast(e.outcome == domain::AuctionOutcome::Sold
                        ? "Bạn đã chốt phiên «" + title + "» với giá " + Formatters::formatMoney(e.finalPrice)
                        : "Người thắng không đủ tiền thanh toán; đã xử lý tịch thu cọc theo quy định.",
                        e.outcome == domain::AuctionOutcome::Sold ? "success" : "danger",
                        "Xem kết quả", open);
                }
                if (joined) {
                    if (e.winnerId == session_.id() && e.outcome == domain::AuctionOutcome::Sold)
                        toasts_->showToast("Chúc mừng! Bạn đã thắng «" + title + "» với giá " +
                                               Formatters::formatMoney(e.finalPrice) +
                                               ". Vật phẩm đã được thêm vào Kho lưu trữ!",
                                           "accent", "Mở kho xem", [this] { navigate(Page::Inventory); });
                    else if (e.winnerId == session_.id() &&
                             e.outcome == domain::AuctionOutcome::PaymentDefault)
                        toasts_->showToast("Không thể thanh toán «" + title +
                                               "». Tiền cọc đã bị tịch thu.",
                                           "danger", "Xem kết quả", open);
                    else
                        toasts_->showToast((e.closedEarly ? "Người bán đã chốt sớm phiên «" + title + "». Đã hoàn cọc "
                                                          : "Phiên đã kết thúc. Đã hoàn cọc ") +
                                               Formatters::formatMoney(a->depositAmount),
                                           "info", "Xem phiên", open);
                }
            } else if constexpr (std::is_same_v<E, domain::StateChangedEvent>) {
                if (e.toState == "Đang diễn ra" &&
                    (joined || session_.follows(e.auctionId.value())))
                    toasts_->showToast("Phiên «" + title + "» đã bắt đầu.", "success", "Xem phiên",
                                       open);
            } else if constexpr (std::is_same_v<E, domain::AuctionCancelledEvent>) {
                if (joined)
                    toasts_->showToast("Phiên bị hủy. Đã hoàn cọc " +
                                           Formatters::formatMoney(a->depositAmount),
                                       "warning", "Xem phiên", open);
            } else if constexpr (std::is_same_v<E, domain::CountdownEvent>) {
                const auto key = QString::number(session_.id().value()) + "/" +
                                 QString::number(e.auctionId.value());
                if (joined && e.secondsRemaining <= 10 && e.secondsRemaining > 0 &&
                    !countdownNotified_.contains(key)) {
                    countdownNotified_.insert(key);
                    toasts_->showToast("«" + title + "» · Còn " +
                                           QString::number(e.secondsRemaining) + " giây.",
                                       "warning", "Xem phiên", open);
                }
            }
        },
        event);
}
void AppShell::status() {
    int count = 0;
    for (const auto &a : store_.auctions())
        count += isLive(a);
    statusBar()->showMessage(
        "LIVE · Realtime đang hoạt động    |    " + QString::number(count) +
        " phiên đang diễn ra    |    " +
        (session_.user ? QString::fromStdString(session_.user->name) : "Chưa chọn người dùng") +
        "    |    v2.0");
}
void AppShell::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (workspace_)
        workspace_->sidebar->setCollapsed(width() < 1280 || manualCollapse_, false);
    if (toasts_)
        toasts_->reposition();
}
} // namespace auction::presentation
