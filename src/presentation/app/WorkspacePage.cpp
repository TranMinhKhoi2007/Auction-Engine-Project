#include "auction/presentation/app/WorkspacePage.h"
#include "auction/presentation/Formatters.h"

#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/util/SoundManager.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QPropertyAnimation>
namespace auction::presentation {
NavSidebar::NavSidebar(UiContext &c, QWidget *parent) : QFrame(parent), c_(c) {
    setObjectName("navSidebar");
    setFixedWidth(232);
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(12, 20, 12, 16);
    main->setSpacing(8);
    names_ = {{Page::Home, "Trang chủ"},           {Page::Marketplace, "Sàn đấu giá"},
              {Page::MyAuctions, "Phiên của tôi"}, {Page::Seller, "Bán hàng"},
              {Page::Inventory, "Kho lưu trữ"},    {Page::Wallet, "Ví"},
              {Page::Notifications, "Thông báo"},  {Page::Log, "Nhật ký hệ thống"}};
    icons_ = {{Page::Home, "home"},          {Page::Marketplace, "gavel"},
              {Page::MyAuctions, "list"},    {Page::Seller, "store"},
              {Page::Inventory, "award"},    {Page::Wallet, "wallet"},
              {Page::Notifications, "bell"}, {Page::Log, "log"}};
    const QMap<Page, QString> objects = {{Page::Home, "navHome"},
                                         {Page::Marketplace, "navMarketplace"},
                                         {Page::MyAuctions, "navMyAuctions"},
                                         {Page::Seller, "navSeller"},
                                         {Page::Inventory, "navInventory"},
                                         {Page::Wallet, "navWallet"},
                                         {Page::Notifications, "navNotifications"},
                                         {Page::Log, "navLog"}};
    auto *logo = button("AE", "sidebarLogo", this, "ghost");
    logo->setFont(Fonts::body(18, QFont::Bold));
    logo->setContentsMargins(0, 0, 0, 0);
    logo->setStyleSheet("padding: 6px;");
    logo->setAccessibleName("Auction Engine · Trang chủ");
    main->addWidget(logo);
    connect(logo, &QPushButton::clicked, this, [this] { emit navigate(Page::Home); });
    main->addSpacing(16);
    for (auto it = names_.cbegin(); it != names_.cend(); ++it) {
        auto *b = button(it.value(), objects.value(it.key()), this, "ghost");
        b->setCheckable(true);
        b->setProperty("nav", true);
        b->setToolTip(it.value());
        buttons_[it.key()] = b;
        main->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, page = it.key()] { emit navigate(page); });
    }
    main->addStretch();
    auto *theme = button("", "sidebarTheme", this, "ghost");
    theme->setIcon(IconProvider::icon("sun", QColor(c.theme.theme().textMuted)));
    theme->setToolTip("Đổi giao diện sáng / tối");
    theme->setAccessibleName(theme->toolTip());
    main->addWidget(theme);
    connect(theme, &QPushButton::clicked, this,
            [this] { c_.theme.setLight(!c_.theme.theme().light); });
    auto *collapse = button("", "sidebarCollapse", this, "ghost");
    collapse->setToolTip("Thu / mở thanh điều hướng · Ctrl+B");
    collapse->setAccessibleName(collapse->toolTip());
    collapse->setIcon(IconProvider::icon("panel-left", QColor(c.theme.theme().textMuted)));
    main->addWidget(collapse);
    connect(collapse, &QPushButton::clicked, this, &NavSidebar::toggleRequested);
    connect(&c.session, &UserSession::changed, this, &NavSidebar::refresh);
    connect(&c.theme, &ThemeManager::changed, this, [this, theme, collapse] {
        theme->setIcon(IconProvider::icon(c_.theme.theme().light ? "moon" : "sun",
                                          QColor(c_.theme.theme().textMuted)));
        collapse->setIcon(IconProvider::icon("panel-left", QColor(c_.theme.theme().textMuted)));
        refresh();
    });
    refresh();
}
void NavSidebar::refresh() {
    for (auto it = buttons_.cbegin(); it != buttons_.cend(); ++it) {
        it.value()->setText(collapsed_ ? QString() : names_.value(it.key()));
        it.value()->setIcon(
            IconProvider::icon(icons_.value(it.key()), QColor(c_.theme.theme().textMuted)));
        it.value()->setIconSize({20, 20});
    }
    buttons_[Page::Seller]->setVisible(c_.session.loggedIn());
    buttons_[Page::Inventory]->setVisible(c_.session.loggedIn());
}
void NavSidebar::setCollapsed(bool value, bool animate) {
    if (collapsed_ == value && maximumWidth() == (value ? 72 : 232))
        return;
    collapsed_ = value;
    refresh();
    if (!animate) {
        setFixedWidth(value ? 72 : 232);
        return;
    }
    setMinimumWidth(72);
    auto *animation = new QPropertyAnimation(this, "maximumWidth", this);
    animation->setStartValue(width());
    animation->setEndValue(value ? 72 : 232);
    animation->setDuration(180);
    connect(animation, &QPropertyAnimation::valueChanged, this,
            [this](const QVariant &v) { setMinimumWidth(v.toInt()); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
void NavSidebar::select(Page page) {
    for (auto it = buttons_.cbegin(); it != buttons_.cend(); ++it)
        it.value()->setChecked(it.key() == page ||
                               (page == Page::Detail && it.key() == Page::Marketplace));
}
TopBar::TopBar(UiContext &c, QWidget *parent) : QFrame(parent), c_(c) {
    setObjectName("topBar");
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(20, 12, 20, 12);
    row->setSpacing(12);
    title_ = label("Sàn đấu giá", this, 16);
    title_->setMinimumWidth(120);
    row->addWidget(title_);
    search = new QLineEdit(this);
    search->setObjectName("edtSearch");
    search->setAccessibleName("Tìm phiên theo tên, mã hoặc danh mục");
    search->setPlaceholderText("Tìm phiên…  Ctrl+K");
    search->setMinimumWidth(140);
    search->setMaximumWidth(320);
    row->addWidget(search, 1);
    results_ = new QStringListModel(this);
    completer_ = new QCompleter(results_, this);
    completer_->setFilterMode(Qt::MatchContains);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    search->setCompleter(completer_);
    connect(completer_, qOverload<const QString &>(&QCompleter::activated), this,
            [this](const QString &text) {
                if (searchIds_.contains(text))
                    emit openRequested(searchIds_[text]);
            });
    connect(search, &QLineEdit::returnPressed, this, [this] {
        const auto query = search->text();
        for (auto it = searchIds_.cbegin(); it != searchIds_.cend(); ++it)
            if (it.key().contains(query, Qt::CaseInsensitive)) {
                emit openRequested(it.value());
                break;
            }
    });
    auto *clock = new ClockChip(c.ticker, this);
    clock->setObjectName("lblClock");
    row->addWidget(clock);
    balance_ = new BalanceChip(this);
    balance_->setObjectName("balanceChip");
    row->addWidget(balance_);
    bell = button("", "btnNotifications", this, "ghost");
    bell->setToolTip("Thông báo gần đây");
    bell->setAccessibleName(bell->toolTip());
    row->addWidget(bell);
    notificationBadge = label("0", this, 12, "accent");
    notificationBadge->setObjectName("notifBadge");
    row->addWidget(notificationBadge);
    soundToggle = button(QString::fromUtf8("🔊"), "btnSoundToggle", this, "ghost");
    soundToggle->setFixedWidth(38);
    soundToggle->setAccessibleName("Bật/Tắt âm thanh");
    auto updateSoundUi = [this] {
        const bool muted = SoundManager::instance().isMuted();
        soundToggle->setText(muted ? QString::fromUtf8("🔇") : QString::fromUtf8("🔊"));
        soundToggle->setToolTip(muted ? "Âm thanh: Đang tắt (Bấm để bật)" : "Âm thanh: Đang bật (Bấm để tắt)");
    };
    updateSoundUi();
    connect(soundToggle, &QPushButton::clicked, this, [updateSoundUi] {
        SoundManager::instance().setMuted(!SoundManager::instance().isMuted());
        updateSoundUi();
    });
    connect(&SoundManager::instance(), &SoundManager::muteChanged, this, [updateSoundUi](bool) {
        updateSoundUi();
    });
    row->addWidget(soundToggle);
    profile_ = button("Đăng nhập", "btnProfile", this);
    profile_->setFixedWidth(150);
    profileMenu = new QMenu(profile_);
    profile_->setMenu(profileMenu);
    row->addWidget(profile_);
    connect(profileMenu, &QMenu::aboutToShow, this, [this] {
        profileMenu->clear();
        const auto own=c_.session.user;
        auto *info=profileMenu->addAction(own?QString::fromStdString(own->name):"Tài khoản đấu giá");
        info->setEnabled(false);info->setToolTip(info->text());
        if(own) {
            const auto email=QString::fromStdString(own->email);
            auto *address=profileMenu->addAction(profileMenu->fontMetrics().elidedText(email,Qt::ElideMiddle,300));
            address->setObjectName("profileEmail");address->setToolTip(email);address->setEnabled(false);
            profileMenu->setToolTipsVisible(true);
        }
        profileMenu->addSeparator();
        auto *change = profileMenu->addAction(c_.session.loggedIn() ? "Đổi tài khoản" : "Đăng nhập / Đăng ký");
        change->setObjectName("actionSwitchAccount");
        connect(change, &QAction::triggered, this, &TopBar::createUserRequested);
        auto *logout = profileMenu->addAction("Đăng xuất");
        logout->setObjectName("actionLogout");
        logout->setEnabled(c_.session.loggedIn());
        connect(logout, &QAction::triggered, this, &TopBar::logoutRequested);
        auto *topUp = profileMenu->addAction("Nạp tiền");
        topUp->setObjectName("actionTopUp");
        topUp->setEnabled(c_.session.user.has_value() && c_.store.canWrite());
        connect(topUp, &QAction::triggered, this, &TopBar::topUpRequested);
        auto *inv = profileMenu->addAction("Kho lưu trữ vật phẩm");
        inv->setObjectName("actionInventory");
        inv->setEnabled(c_.session.loggedIn());
        connect(inv, &QAction::triggered, this, &TopBar::inventoryRequested);
        auto *changePwd = profileMenu->addAction("Đổi mật khẩu");
        changePwd->setObjectName("actionChangePassword");
        changePwd->setEnabled(c_.session.loggedIn());
        connect(changePwd, &QAction::triggered, this, &TopBar::changePasswordRequested);
        profileMenu->addSeparator();
        auto *light = profileMenu->addAction("Giao diện sáng");
        light->setObjectName("actionLightTheme");
        light->setCheckable(true);
        light->setChecked(c_.theme.theme().light);
        connect(light, &QAction::toggled, &c_.theme, &ThemeManager::setLight);
    });
    connect(&c.store, &AuctionStore::peopleChanged, this, &TopBar::refresh);
    connect(&c.store, &AuctionStore::personalChanged, this, &TopBar::refresh);
    connect(&c.store, &AuctionStore::auctionListChanged, this, &TopBar::refresh);
    connect(&c.session, &UserSession::changed, this, &TopBar::refresh);
    connect(&c.theme, &ThemeManager::changed, this, &TopBar::refresh);
    refresh();
}
void TopBar::refresh() {
    const auto name = c_.session.user ? QString::fromStdString(c_.session.user->name) : "Đăng nhập";
    profile_->setText(profile_->fontMetrics().elidedText(name, Qt::ElideRight, 100));
    profile_->setToolTip(name);
    const auto wallet = c_.store.wallet();
    balance_->setVisible(wallet.has_value());
    if (wallet) {
        balance_->setBalance("Khả dụng: " + Formatters::formatMoney(wallet->available),
                            "Đang ký quỹ: " + Formatters::formatMoney(wallet->totalHeld) +
                            "\nTổng: " + Formatters::formatMoney(wallet->total));
    }
    profile_->setIcon(IconProvider::icon("user", QColor(c_.theme.theme().accent)));
    bell->setIcon(IconProvider::icon("bell", QColor(c_.theme.theme().textMuted)));
    search->setClearButtonEnabled(true);
    searchIds_.clear();
    for (const auto &a : c_.store.auctions())
        searchIds_[QString::fromStdString(a.productName) + " · #" + QString::number(a.id.value()) +
                   " · " + QString::fromStdString(a.category)] = a.id.value();
    results_->setStringList(searchIds_.keys());
}
void TopBar::setTitle(const QString &title) {
    title_->setText(title);
}
WorkspacePage::WorkspacePage(UiContext &c, QWidget *parent) : QWidget(parent) {
    setObjectName("workspacePage");
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);
    sidebar = new NavSidebar(c, this);
    row->addWidget(sidebar);
    auto *right = new QVBoxLayout;
    right->setContentsMargins(0, 0, 0, 0);
    right->setSpacing(0);
    topbar = new TopBar(c, this);
    right->addWidget(topbar);
    pages = new QStackedWidget(this);
    right->addWidget(pages, 1);
    row->addLayout(right, 1);
}
} // namespace auction::presentation
