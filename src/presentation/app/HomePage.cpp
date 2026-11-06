#include "auction/presentation/app/HomePage.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/util/UiHelpers.h"

#include <algorithm>
namespace auction::presentation {
HomePage::HomePage(UiContext &c, QWidget *parent) : QScrollArea(parent), c_(c) {
    setObjectName("homePage");
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *outer = new QWidget(this);
    outer->setObjectName("homeContent");
    auto *center = new QHBoxLayout(outer);
    center->setContentsMargins(24, 16, 24, 32);
    center->addStretch();
    auto *body = new QWidget(outer);
    body->setMaximumWidth(1200);
    auto *main = new QVBoxLayout(body);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(28);
    center->addWidget(body, 1);
    center->addStretch();
    setWidget(outer);
    auto *top = new QHBoxLayout;
    auto *logo = button("Auction Engine", "homeLogo", body, "ghost");
    logo->setFont(Fonts::body(20, QFont::Bold));
    top->addWidget(logo);
    top->addStretch();
    const auto updateLogo = [this, logo] {
        logo->setIcon(IconProvider::icon("gavel", QColor(c_.theme.theme().accent), 24));
    };
    connect(&c.theme, &ThemeManager::changed, this, updateLogo);
    updateLogo();
    auto *how = button("Cách hoạt động", "homeHow", body, "ghost");
    auto *highlights = button("Điểm nổi bật", "homeHighlights", body, "ghost");
    auto *about = button("Về dự án", "homeAbout", body, "ghost");
    top->addWidget(how);
    top->addWidget(highlights);
    top->addWidget(about);
    auto *clock = new ClockChip(c.ticker, body);
    clock->setObjectName("lblHomeClock");
    top->addWidget(clock);
    auto *enter = button("Vào sàn đấu giá", "btnEnterMarketplace", body, "primary");
    connect(enter, &QPushButton::clicked, this, &HomePage::enterRequested);
    top->addWidget(enter);
    main->addLayout(top);
    auto *hero = new QHBoxLayout;
    hero->setSpacing(24);
    auto *copy = new QWidget(body);
    auto *copyLayout = new QVBoxLayout(copy);
    copyLayout->setContentsMargins(0, 16, 0, 16);
    copyLayout->setSpacing(16);

    auto *pillBadge = new QLabel("⚡ SÀN ĐẤU GIÁ THỜI GIAN THỰC 2.0  ·  C++20 & QT 6", copy);
    pillBadge->setTextFormat(Qt::PlainText);
    pillBadge->setWordWrap(false);
    pillBadge->setFont(Fonts::body(11, QFont::Bold));
    pillBadge->setFixedHeight(26);
    pillBadge->setStyleSheet(
        "background: rgba(234, 179, 8, 0.15); border: 1px solid rgba(234, 179, 8, 0.4); "
        "color: #eab308; padding: 0px 14px; border-radius: 13px; font-weight: bold; font-size: 11px;");
    copyLayout->addWidget(pillBadge, 0, Qt::AlignLeft);

    auto *headline = label("Đặt giá từng mili-giây.\nMinh bạch từng khoản ký quỹ.", copy, 32);
    headline->setFont(Fonts::body(32, QFont::Bold));
    copyLayout->addWidget(headline);

    auto *subtitle = label(
        "Nền tảng đấu giá trực tuyến hiệu năng cao với State Pattern đa trạng thái, "
        "cơ chế chống bắn tỉa (anti-sniping) và âm thanh gõ búa chốt phiên kịch tính.",
        copy, 14, "muted");
    subtitle->setWordWrap(true);
    copyLayout->addWidget(subtitle);

    auto *trustBadges = new QHBoxLayout;
    trustBadges->setSpacing(8);
    const QStringList badgeTexts = {
        "🛡️ Ký quỹ tự động", "⚡ Anti-sniping 60s", "🔨 Gõ búa chốt phiên"
    };
    for (const auto &btext : badgeTexts) {
        auto *bLabel = new QLabel(btext, copy);
        bLabel->setTextFormat(Qt::PlainText);
        bLabel->setWordWrap(false);
        bLabel->setFont(Fonts::body(12, QFont::Medium));
        bLabel->setFixedHeight(28);
        bLabel->setStyleSheet(
            "background: rgba(127, 127, 127, 0.12); border-radius: 6px; padding: 0px 10px; font-weight: 500;");
        trustBadges->addWidget(bLabel);
    }
    trustBadges->addStretch();
    copyLayout->addLayout(trustBadges);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(10);
    auto *start = button("Vào sàn đấu giá  →", "heroEnter", copy, "primary");
    start->setFixedHeight(40);
    auto *createAuctionBtn = button("Tạo phiên đấu giá  +", "heroCreateAuction", copy, "ghost");
    createAuctionBtn->setFixedHeight(40);
    createAuctionBtn->setToolTip("Tạo phiên đấu giá mới để đăng bán vật phẩm của bạn");
    connect(createAuctionBtn, &QPushButton::clicked, this, [this] { emit quickStartRequested(2); });
    auto *learn = button("Xem cách hoạt động", "heroHow", copy, "ghost");
    learn->setFixedHeight(40);
    actions->addWidget(start);
    actions->addWidget(createAuctionBtn);
    actions->addWidget(learn);
    actions->addStretch();
    copyLayout->addLayout(actions);
    connect(start, &QPushButton::clicked, this, &HomePage::enterRequested);

    copyLayout->addWidget(
        label("Đồng hồ SteadyTime chính xác  ·  Ký quỹ minh bạch  ·  Nhật ký kiểm toán đầy đủ", copy, 12, "muted"));
    copyLayout->addStretch();
    hero->addWidget(copy, 1);

    featureHost_ = new QWidget(body);
    featureLayout_ = new QVBoxLayout(featureHost_);
    featureLayout_->setContentsMargins(0, 0, 0, 0);
    featureTitleLabel_ = label("PHIÊN NỔI BẬT", featureHost_, 12, "accent");
    featureTitleLabel_->setFont(Fonts::body(12, QFont::Bold));
    featureLayout_->addWidget(featureTitleLabel_);
    featureEmpty_ = new EmptyState("Chưa có phiên đấu giá nào",
                                   "Tạo tài khoản và khám phá trải nghiệm đấu giá đầu tiên.",
                                   "Bắt đầu nhanh", c.theme, featureHost_);
    featureEmpty_->setMinimumHeight(320);
    featureLayout_->addWidget(featureEmpty_);
    connect(featureEmpty_->action, &QPushButton::clicked, this,
            [this] { ensureWidgetVisible(gettingStarted_); });
    hero->addWidget(featureHost_, 1);
    main->addLayout(hero);

    auto *statRow = new QHBoxLayout;
    QStringList titles = {"Phiên đang diễn ra", "Người dùng", "Tổng lượt đặt giá",
                          "Giá trị đã giao dịch"};
    QStringList icons = {"activity", "users", "trending-up", "wallet"};
    for (int i = 0; i < 4; ++i) {
        auto *stat = new StatCard(titles[i], icons[i], c.theme, body);
        stats_ << stat;
        statRow->addWidget(stat, 1);
    }
    stats_[0]->value->setObjectName("statLiveAuctions");
    main->addLayout(statRow);

    closing_ = new QWidget(body);
    auto *closingVBox = new QVBoxLayout(closing_);
    closingVBox->setContentsMargins(0, 0, 0, 0);
    closingVBox->setSpacing(8);

    auto *headerRow = new QHBoxLayout;
    closingTitleLabel_ = label("Khám phá các phiên trên sàn", closing_, 22);
    closingTitleLabel_->setFont(Fonts::body(22, QFont::Bold));
    headerRow->addWidget(closingTitleLabel_);
    headerRow->addStretch();
    auto *viewAllBtn = button("Xem tất cả trên sàn →", "btnViewAllAuctions", closing_, "ghost");
    connect(viewAllBtn, &QPushButton::clicked, this, &HomePage::enterRequested);
    headerRow->addWidget(viewAllBtn);
    closingVBox->addLayout(headerRow);

    auto *grid = new QWidget(closing_);
    closingLayout_ = new FlowLayout(grid);
    closingVBox->addWidget(grid);
    main->addWidget(closing_);
    gettingStarted_ = new Card(body);
    gettingStarted_->content->addWidget(label("Bắt đầu nhanh", gettingStarted_, 20));
    gettingStarted_->content->addWidget(label(
        "Hoàn thành 5 bước để trải nghiệm một vòng đấu giá đầy đủ.", gettingStarted_, 14, "muted"));
    const QStringList steps = {"Tạo tài khoản", "Nạp tiền", "Tạo phiên đấu giá",
                                "Duyệt phiên (chủ phiên)", "Dùng tài khoản khác tham gia và đặt giá"};
    for (int i = 0; i < steps.size(); ++i) {
        auto *b = button(QString::number(i + 1) + ".  " + steps[i],
                         "quickStep" + QString::number(i), gettingStarted_, "ghost");
        b->setProperty("nav", true);
        steps_ << b;
        gettingStarted_->content->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, i] { emit quickStartRequested(i); });
    }
    main->addWidget(gettingStarted_);
    auto *howCard = new Card(body);
    howCard->content->addWidget(label("Cách hoạt động", howCard, 28));
    auto *howRow = new QHBoxLayout;
    QStringList instructions = {"Đăng ký & nạp tiền", "Đặt cọc để tham gia",
                                "Đặt giá thời gian thực", "Quyết toán & hoàn cọc"};
    for (int i = 0; i < 4; ++i) {
        auto *w = new QWidget(howCard);
        auto *v = new QVBoxLayout(w);
        v->addWidget(label("0" + QString::number(i + 1), w, 28, "accent"));
        v->addWidget(label(instructions[i], w, 16));
        howRow->addWidget(w, 1);
    }
    howCard->content->addLayout(howRow);
    howCard->content->addWidget(
        label("Tiền cọc là khoản cố định theo từng phiên, không phụ thuộc giá bạn đặt. Thắng thì "
              "cọc được trừ vào giá; thua hoặc phiên hủy thì được hoàn cọc.",
              howCard, 14, "muted"));
    main->addWidget(howCard);
    auto *featureSection = new QWidget(body);
    auto *fv = new QVBoxLayout(featureSection);
    fv->setContentsMargins(0, 0, 0, 0);
    fv->addWidget(label("Được xây dựng cho sự minh bạch", featureSection, 28));
    auto *fr = new QHBoxLayout;
    const QStringList ft = {"Realtime chính xác", "Ký quỹ minh bạch", "Nhật ký audit đầy đủ"};
    const QStringList fd = {"Đồng hồ đơn điệu điều phối từng phiên. Giao diện cập nhật liên tục.",
                            "Tiền cọc không thay đổi khi bạn đặt giá hoặc bị vượt giá.",
                            "Mọi lần tham gia, đặt giá và hoàn tiền đều có dấu vết."};
    for (int i = 0; i < 3; ++i) {
        auto *card = new Card(featureSection);
        card->content->addWidget(label(ft[i], card, 20));
        card->content->addWidget(label(fd[i], card, 14, "muted"));
        fr->addWidget(card, 1);
    }
    fv->addLayout(fr);
    main->addWidget(featureSection);
    connect(how, &QPushButton::clicked, this, [this, howCard] { ensureWidgetVisible(howCard); });
    connect(learn, &QPushButton::clicked, how, &QPushButton::click);
    connect(highlights, &QPushButton::clicked, this,
            [this, featureSection] { ensureWidgetVisible(featureSection); });
    connect(about, &QPushButton::clicked, this, [this, featureSection] { ensureWidgetVisible(featureSection); });
    connect(&c.store, &AuctionStore::auctionChanged, this, [this](quint64) { refresh(); });
    connect(&c.store, &AuctionStore::peopleChanged, this, &HomePage::refresh);
    connect(&c.store, &AuctionStore::personalChanged, this, &HomePage::refresh);
    refresh();
}
void HomePage::refresh() {
    QList<quint64> live;
    QList<quint64> upcoming;
    QList<quint64> pending;
    QList<quint64> others;
    quint64 bids = 0;
    domain::Money sales = domain::Money::zero();
    bool approved = false, joined = false;

    for (const auto &a : c_.store.auctions()) {
        if (isLive(a)) {
            live << a.id.value();
        } else if (a.state == "Sắp diễn ra") {
            upcoming << a.id.value();
        } else if (a.state == "Chờ duyệt") {
            pending << a.id.value();
        } else {
            others << a.id.value();
        }
        bids += a.bidHistory.size();
        if (a.outcome == domain::AuctionOutcome::Sold)
            sales = sales + a.currentPrice;
        approved |= a.state != "Chờ duyệt";
        joined |= !a.bidHistory.empty();
    }

    std::sort(live.begin(), live.end(), [this](quint64 a, quint64 b) {
        return c_.store.endSteady(a) < c_.store.endSteady(b);
    });

    stats_[0]->value->setText(QString::number(live.size()));
    stats_[1]->value->setText(QString::number(c_.store.accountCount()));
    stats_[2]->value->setText(QString::number(bids));
    stats_[3]->value->setText(Formatters::formatMoney(sales));

    // Lựa chọn Phiên nổi bật (Featured Auction):
    // Ưu tiên: 1. Live -> 2. Sắp diễn ra -> 3. Chờ duyệt (mới tạo) -> 4. Khác
    quint64 feature = 0;
    QString featureTitle = "PHIÊN NỔI BẬT";
    if (!live.isEmpty()) {
        feature = live.first();
        featureTitle = "🔥 PHIÊN ĐẤU GIÁ NỔI BẬT · ĐANG DIỄN RA";
    } else if (!upcoming.isEmpty()) {
        feature = upcoming.first();
        featureTitle = "⏰ PHIÊN ĐẤU GIÁ NỔI BẬT · SẮP BẮT ĐẦU";
    } else if (!pending.isEmpty()) {
        feature = pending.first();
        featureTitle = "⏳ PHIÊN MỚI TẠO · CHỜ DUYỆT";
    } else if (!others.isEmpty()) {
        feature = others.first();
        featureTitle = "🏁 PHIÊN ĐẤU GIÁ GẦN ĐÂY";
    }

    if (featureTitleLabel_) {
        featureTitleLabel_->setText(featureTitle);
    }

    if (feature != featuredId_) {
        if (featured_) {
            featureLayout_->removeWidget(featured_);
            delete featured_;
            featured_ = nullptr;
        }
        featuredId_ = feature;
        if (feature) {
            featured_ = new AuctionCard(feature, c_.store, c_.session, c_.ticker, c_.theme,
                                        c_.images, featureHost_);
            featured_->setCompact(false);
            featureLayout_->addWidget(featured_);
            connect(featured_, &AuctionCard::openRequested, this, &HomePage::openRequested);
        }
    }
    featureEmpty_->setVisible(feature == 0 && c_.store.loaded());

    // Hiển thị danh sách các phiên trên sàn (Grid)
    QList<quint64> displayList;
    displayList << live << upcoming << pending << others;

    closing_->setVisible(!displayList.isEmpty());
    if (closingTitleLabel_) {
        closingTitleLabel_->setText(!live.isEmpty()
            ? "🔥 Phiên đang diễn ra & sắp kết thúc (" + QString::number(displayList.size()) + ")"
            : "🔥 Khám phá các phiên trên sàn (" + QString::number(displayList.size()) + ")");
    }

    for (auto *card : cards_)
        card->hide();

    // Hiển thị tối đa 6 phiên trên grid
    const int count = std::min(6, static_cast<int>(displayList.size()));
    for (int i = 0; i < count; ++i) {
        auto id = displayList[i];
        if (!cards_.contains(id)) {
            auto *card =
                new AuctionCard(id, c_.store, c_.session, c_.ticker, c_.theme, c_.images, closing_);
            cards_[id] = card;
            closingLayout_->addWidget(card);
            connect(card, &AuctionCard::openRequested, this, &HomePage::openRequested);
        }
        cards_[id]->show();
    }

    bool funded = c_.session.user && c_.store.wallet() && c_.store.wallet()->available.amount() + c_.store.wallet()->totalHeld.amount() > 0;
    for (const auto &a : c_.store.auctions())
        funded |= !a.participants.empty(); // public proof of a paid deposit, never another user's wallet.
    const bool complete[] = {c_.store.accountCount()>0, funded,
                             !c_.store.auctions().isEmpty(), approved, joined};
    bool all = true;
    for (int i = 0; i < 5; ++i) {
        steps_[i]->setIcon(IconProvider::icon(
            complete[i] ? "check" : "arrow-right",
            QColor(complete[i] ? c_.theme.theme().successText : c_.theme.theme().textMuted)));
        all &= complete[i];
    }
    gettingStarted_->setVisible(!all);
}
} // namespace auction::presentation
