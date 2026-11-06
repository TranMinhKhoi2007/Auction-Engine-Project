#include "auction/presentation/pages/MarketplacePage.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QButtonGroup>
#include <algorithm>
namespace auction::presentation {
MarketplacePage::MarketplacePage(UiContext &c, QWidget *parent, Mode mode)
    : QWidget(parent), c_(c), mode_(mode) {
    list_ = mode_ != Mode::Market;
    setObjectName("marketplacePage");
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(24, 24, 24, 24);
    main->setSpacing(16);
    const QStringList titles = {"Khám phá sàn đấu giá", "Phiên của tôi", "Không gian bán hàng"};
    auto *heading = new QHBoxLayout;
    auto *headingText = new QVBoxLayout;
    headingText->addWidget(label(titles[static_cast<int>(mode)], this, 28));
    headingText->addWidget(label("Giá trị thật. Diễn biến trực tiếp. Quyết định của bạn.",
                                 this, 14, "muted"));
    heading->addLayout(headingText, 1);
    create_ = button("Tạo phiên đấu giá", "btnCreateAuction", this, "primary");
    heading->addWidget(create_);
    connect(create_, &QPushButton::clicked, this, &MarketplacePage::createRequested);
    seedDemo_ = button("⚡ Nạp phiên mẫu", "btnSeedDemoAuctions", this, "ghost");
    seedDemo_->setToolTip("Tự động tạo các phiên mẫu (Cổ vật, Tranh, Thiết bị) ở trạng thái 'Chờ duyệt' để thuyết trình");
    heading->addWidget(seedDemo_);
    connect(seedDemo_, &QPushButton::clicked, this, &MarketplacePage::seedDemoRequested);
    main->addLayout(heading);
    if (mode == Mode::Seller) {
        auto *row = new QHBoxLayout;
        QStringList labels = {"Phiên đang bán", "Chờ duyệt", "Số dư khả dụng", "Lượt đặt giá"};
        for (const auto &t : labels) {
            auto *s = new StatCard(t, "activity", c.theme, this);
            stats_ << s;
            row->addWidget(s, 1);
        }
        main->addLayout(row);
    }
    tabs_ = new QTabBar(this);
    tabs_->setExpanding(false);
    for (const auto &t : {"Đang tham gia", "Đang theo dõi", "Đã thắng", "Đã kết thúc / Hoàn cọc"})
        tabs_->addTab(QString::fromUtf8(t));
    main->addWidget(tabs_);
    tabs_->setVisible(mode == Mode::Mine);
    connect(tabs_, &QTabBar::currentChanged, this, [this] { refresh(); });
    auto *filterRow = new QHBoxLayout;
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    const QStringList states = {"Tất cả",    "Đang diễn ra", "Sắp diễn ra",
                                "Chờ duyệt", "Kết thúc",     "Hủy"};
    for (const auto &s : states) {
        auto *chip = new Chip(s, this);
        chips_ << chip;
        group->addButton(chip);
        filterRow->addWidget(chip);
        connect(chip, &QPushButton::clicked, this, [this, s] {
            state_ = s == "Tất cả" ? QString() : s;
            refresh();
        });
    }
    chips_.first()->setChecked(true);
    filterRow->addStretch();
    main->addLayout(filterRow);
    auto *tools = new QHBoxLayout;
    category_ = new QComboBox(this);
    category_->setAccessibleName("Danh mục");
    category_->addItems({"Tất cả danh mục", "Đồ cổ", "Tranh nghệ thuật", "Thiết bị công nghệ"});
    sort_ = new QComboBox(this);
    sort_->setAccessibleName("Sắp xếp phiên");
    sort_->addItems({"Sắp kết thúc", "Giá cao → thấp", "Mới nhất"});
    tools->addWidget(category_);
    tools->addWidget(sort_);
    tools->addStretch();
    auto *view = button(list_ ? "Dạng lưới" : "Dạng danh sách", "btnListMode", this, "ghost");
    tools->addWidget(view);
    main->addLayout(tools);
    connect(category_, &QComboBox::currentIndexChanged, this, [this] { refresh(); });
    connect(sort_, &QComboBox::currentIndexChanged, this, [this] { refresh(); });
    connect(view, &QPushButton::clicked, this, [this, view] {
        list_ = !list_;
        view->setText(list_ ? "Dạng lưới" : "Dạng danh sách");
        flow_->setListMode(list_);
        for (auto *card : cards_)
            card->setCompact(list_);
    });
    auto *body = new QWidget(this);
    body->setObjectName("pageBody");
    auto *content = new QVBoxLayout(body);
    content->setContentsMargins(0, 0, 0, 0);
    grid_ = new QWidget(body);
    grid_->setObjectName("auctionGrid");
    flow_ = new FlowLayout(grid_);
    flow_->setListMode(list_);
    content->addWidget(grid_);
    empty_ = new EmptyState("Không có phiên nào phù hợp",
                            "Thử thay đổi bộ lọc hoặc tạo phiên đấu giá đầu tiên.", "Xóa bộ lọc",
                            c.theme, body);
    content->addWidget(empty_);
    content->addStretch();
    main->addWidget(scrollPage(body, this), 1);
    connect(empty_->action, &QPushButton::clicked, this, [this] {
        if (c_.store.accountCount()==0) {
            emit createUserRequested();
            return;
        }
        state_.clear();
        search_.clear();
        category_->setCurrentIndex(0);
        chips_.first()->setChecked(true);
        refresh();
    });
    connect(&c.store, &AuctionStore::auctionChanged, this, [this](quint64) { refresh(); });
    connect(&c.store, &AuctionStore::peopleChanged, this, &MarketplacePage::refresh);
    connect(&c.store, &AuctionStore::personalChanged, this, &MarketplacePage::refresh);
    connect(&c.session, &UserSession::changed, this, &MarketplacePage::refresh);
    refresh();
}
void MarketplacePage::setSearch(const QString &value) {
    search_ = value;
    refresh();
}
void MarketplacePage::refresh() {
    create_->setEnabled(c_.store.canWrite());
    create_->setToolTip(c_.store.canWrite()?QString():"Mất kết nối hoặc đang chờ máy chủ");
    create_->setVisible(c_.session.loggedIn() && mode_ != Mode::Mine);
    if (seedDemo_) {
        seedDemo_->setEnabled(c_.store.canWrite());
        seedDemo_->setToolTip(c_.store.canWrite()?QString():"Mất kết nối hoặc đang chờ máy chủ");
        seedDemo_->setVisible(c_.session.loggedIn() && mode_ != Mode::Mine);
    }
    QList<quint64> visible;
    QMap<QString, int> counts;
    int ownLive = 0, pending = 0;
    quint64 bids = 0;
    domain::Money sales = domain::Money::zero();
    for (const auto &a : c_.store.auctions()) {
        bool scope = true;
        if (mode_ == Mode::Seller)
            scope = c_.session.loggedIn() && a.sellerId == c_.session.id();
        if (mode_ == Mode::Mine) {
            const bool joined = hasJoined(a, c_.session.id());
            switch (tabs_->currentIndex()) {
            case 0:
                scope = joined && !isClosed(a);
                break;
            case 1:
                scope = c_.session.follows(a.id.value());
                break;
            case 2:
                scope = a.outcome == domain::AuctionOutcome::Sold && a.leaderId == c_.session.id();
                break;
            default:
                scope = joined && isClosed(a);
            }
        }
        if (!scope)
            continue;
        ++counts[QString::fromStdString(a.state)];
        ++counts["Tất cả"];
        ownLive += !isClosed(a);
        pending += a.state == "Chờ duyệt";
        bids += a.bidHistory.size();
        if (a.outcome == domain::AuctionOutcome::Sold)
            sales = sales + a.currentPrice;
        if (!cards_.contains(a.id.value())) {
            auto *card = new AuctionCard(a.id.value(), c_.store, c_.session, c_.ticker, c_.theme,
                                         c_.images, grid_);
            card->hide();
            card->setCompact(list_);
            card->setManagement(mode_ == Mode::Seller,
                                mode_ == Mode::Seller);
            cards_[a.id.value()] = card;
            connect(card, &AuctionCard::openRequested, this, &MarketplacePage::openRequested);
            connect(card, &AuctionCard::approveRequested, this, &MarketplacePage::approveRequested);
            connect(card, &AuctionCard::cancelRequested, this, &MarketplacePage::cancelRequested);
        }
        auto text = QString::fromStdString(a.productName + " " + a.category) + " #" +
                    QString::number(a.id.value());
        if ((state_.isEmpty() || QString::fromStdString(a.state) == state_) &&
            (category_->currentIndex() == 0 ||
             QString::fromStdString(a.category) == category_->currentText()) &&
            (search_.isEmpty() || text.contains(search_, Qt::CaseInsensitive)))
            visible << a.id.value();
    }
    const QStringList states = {"Tất cả",    "Đang diễn ra", "Sắp diễn ra",
                                "Chờ duyệt", "Kết thúc",     "Hủy"};
    for (int i = 0; i < states.size(); ++i)
        chips_[i]->setText(states[i] + "  " + QString::number(counts.value(states[i])));
    if (!stats_.isEmpty()) {
        stats_[0]->value->setText(
            QString::number(mode_ == Mode::Seller ? ownLive : counts.value("Tất cả")));
        stats_[1]->value->setText(QString::number(pending));
        stats_[2]->value->setText(Formatters::formatMoney(
            mode_ == Mode::Seller && c_.store.wallet() ? c_.store.wallet()->available : sales));
        stats_[3]->value->setText(QString::number(bids));
    }
    std::sort(visible.begin(), visible.end(), [this](quint64 a, quint64 b) {
        if (sort_->currentIndex() == 1)
            return c_.store.get(a)->currentPrice > c_.store.get(b)->currentPrice;
        if (sort_->currentIndex() == 2)
            return a > b;
        return c_.store.endSteady(a) < c_.store.endSteady(b);
    });
    if (visible != order_) {
        while (auto *item = flow_->takeAt(0))
            delete item;
        for (auto *card : cards_)
            card->hide();
        for (auto id : visible) {
            flow_->addWidget(cards_[id]);
            cards_[id]->show();
        }
        order_ = visible;
        flow_->invalidate();
    }
    empty_->setVisible(visible.isEmpty() && c_.store.loaded());
    grid_->setVisible(!visible.isEmpty());
    if (c_.store.accountCount()==0) {
        auto labels = empty_->findChildren<QLabel *>();
        if (labels.size() >= 3) {
            labels[1]->setText("Tạo người dùng đầu tiên");
            labels[2]->setText("Thêm người bán hoặc người tham gia để bắt đầu.");
        }
        empty_->action->setText("Thêm người dùng");
    } else {
        auto labels = empty_->findChildren<QLabel *>();
        if (labels.size() >= 3) {
            labels[1]->setText("Không có phiên nào phù hợp");
            labels[2]->setText(
                mode_ == Mode::Mine
                    ? "Các phiên bạn tham gia, theo dõi và đã thắng sẽ xuất hiện ở đây."
                    : "Thử thay đổi bộ lọc hoặc tạo phiên đấu giá đầu tiên.");
        }
        empty_->action->setText("Xóa bộ lọc");
    }
}
} // namespace auction::presentation
