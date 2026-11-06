#include "auction/presentation/pages/AccountPages.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QHeaderView>
#include <QPainter>
#include <deque>
#include <memory>
namespace auction::presentation {
WalletPage::WalletPage(UiContext &c, QWidget *parent) : QWidget(parent), c_(c) {
    setObjectName("walletPage");
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(24, 24, 24, 24);
    main->setSpacing(20);
    auto *title = new QHBoxLayout;
    auto *heading = new QVBoxLayout;
    heading->addWidget(label("Ví của tôi", this, 28));
    heading->addWidget(
        label("Theo dõi số dư và từng khoản ký quỹ, trong một nơi.", this, 14, "muted"));
    title->addLayout(heading, 1);
    topUp_ = button("Nạp tiền", "btnTopUp", this, "primary");
    title->addWidget(topUp_);
    main->addLayout(title);
    connect(topUp_, &QPushButton::clicked, this, &WalletPage::topUpRequested);
    auto *stats = new QHBoxLayout;
    QStringList titles = {"Khả dụng", "Đang ký quỹ", "Tổng tài sản"};
    QStringList names = {"lblWalletAvailable", "lblWalletHeld", "lblWalletTotal"};
    QStringList tones = {"success", "warning", ""};
    for (int i = 0; i < 3; ++i) {
        auto *card = new StatCard(titles[i], "wallet", c.theme, this);
        stats_ << card;
        card->value->setObjectName(names[i]);
        tone(card->value, tones[i]);
        stats->addWidget(card, 1);
    }
    main->addLayout(stats);
    main->addWidget(label("Ký quỹ theo từng phiên", this, 20));
    main->addWidget(
        label("Tiền cọc được giữ nguyên khi bạn đặt giá hoặc bị vượt giá.", this, 14, "muted"));
    table_ = new QTableView(this);
    table_->setObjectName("tblWalletHolds");
    model_ = new QStandardItemModel(0, 4, table_);
    model_->setHorizontalHeaderLabels({"Phiên đấu giá", "Trạng thái", "Tiền cọc", "Thao tác"});
    table_->setModel(model_);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->verticalHeader()->hide();
    table_->verticalHeader()->setDefaultSectionSize(64);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table_->setShowGrid(false);
    main->addWidget(table_, 1);
    connect(table_, &QTableView::clicked, this, [this](const QModelIndex &i) {
        emit openRequested(model_->item(i.row(), 0)->data().toULongLong());
    });
    empty_ = new EmptyState("Bạn chưa đặt cọc cho phiên nào",
                            "Khi tham gia đấu giá, khoản cọc sẽ được hiển thị tại đây.", "",
                            c.theme, this, "wallet");
    main->addWidget(empty_, 1);
    connect(&c.store, &AuctionStore::personalChanged, this, &WalletPage::refresh);
    refresh();
}
void WalletPage::refresh() {
    const auto &wallet = c_.store.wallet();
    topUp_->setEnabled(wallet.has_value() && c_.store.canWrite());
    topUp_->setToolTip(c_.store.canWrite()?QString():"Mất kết nối hoặc đang chờ máy chủ");
    const domain::Money values[] = {wallet ? wallet->available : domain::Money::zero(),
                                    wallet ? wallet->totalHeld : domain::Money::zero(),
                                    wallet ? wallet->total : domain::Money::zero()};
    for (int i = 0; i < 3; ++i)
        stats_[i]->value->setText(Formatters::formatMoney(values[i]));
    int rows = wallet ? static_cast<int>(wallet->holds.size()) : 0;
    model_->setRowCount(rows);
    for (int i = 0; i < rows; ++i) {
        const auto &hold = wallet->holds[static_cast<size_t>(i)];
        const auto *a = c_.store.get(hold.auctionId.value());
        QStringList texts = {"#" + QString::number(hold.auctionId.value()) + " · " +
                                 QString::fromStdString(hold.auctionTitle),
                             a ? QString::fromStdString(a->state) : "",
                             Formatters::formatMoney(hold.amount), "Xem phiên  →"};
        for (int j = 0; j < 4; ++j) {
            auto *item = model_->item(i, j);
            if (!item) {
                auto owned = std::make_unique<QStandardItem>();
                item = owned.get();
                model_->setItem(i, j, owned.release());
            }
            item->setText(j == 1 ? QString() : texts[j]);
            item->setData(texts[j], Qt::AccessibleTextRole);
            item->setToolTip(texts[j]);
            item->setData(QVariant::fromValue<qulonglong>(hold.auctionId.value()));
            if (j == 2)
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        const auto stateIndex = model_->index(i, 1);
        auto *stateHost = table_->indexWidget(stateIndex);
        if (!stateHost) {
            stateHost = new QWidget(table_);
            auto *layout = new QHBoxLayout(stateHost);
            layout->setContentsMargins(10, 4, 10, 4);
            layout->addWidget(new StateBadge(c_.theme, stateHost), 0, Qt::AlignLeft);
            layout->addStretch();
            table_->setIndexWidget(stateIndex, stateHost);
        }
        auto *badge = static_cast<StateBadge *>(stateHost->findChild<QLabel *>("badgeState"));
        badge->setState(texts[1]);
    }
    table_->setVisible(rows > 0);
    empty_->setVisible(rows == 0);
}
namespace {
QString eventKey(const domain::AuctionEvent &e) {
    return std::visit(
        [&](const auto &v) {
            return QString::number(v.auctionId.value()) + "/" + QString::number(e.index()) + "/" +
                   QString::number(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                       v.timestamp.time_since_epoch())
                                       .count());
        },
        e);
}
} // namespace
NotificationsPage::NotificationsPage(UiContext &c, QWidget *parent) : QWidget(parent), c_(c) {
    setObjectName("notificationsPage");
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(24, 24, 24, 24);
    main->setSpacing(16);
    auto *row = new QHBoxLayout;
    row->addWidget(label("Thông báo", this, 28), 1);
    auto *read = button("Đánh dấu đã đọc tất cả", "markAllRead", this);
    row->addWidget(read);
    main->addLayout(row);
    main->addWidget(label("Diễn biến từ các phiên bạn theo dõi và tham gia.", this, 14, "muted"));
    connect(read, &QPushButton::clicked, this, &NotificationsPage::markAllRead);
    list_ = new QListView(this);
    list_->setObjectName("notificationList");
    list_->setWordWrap(true);
    list_->setResizeMode(QListView::Adjust);
    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_->setTextElideMode(Qt::ElideNone);
    list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model_ = new QStandardItemModel(list_);
    list_->setModel(model_);
    main->addWidget(list_, 1);
    empty_ =
        new EmptyState("Chưa có thông báo", "Theo dõi một phiên để nhận các cập nhật mới nhất.", "",
                       c.theme, this, "bell");
    main->addWidget(empty_, 1);
    connect(list_, &QListView::clicked, this,
            [this](const QModelIndex &i) { openLatest(i.row()); });
    connect(&c.store, &AuctionStore::personalChanged, this, &NotificationsPage::refresh);
    connect(&c.theme, &ThemeManager::changed, this, &NotificationsPage::refresh);
    refresh();
}
void NotificationsPage::refresh() {
    const auto &inbox = c_.store.inbox();
    model_->setRowCount(static_cast<int>(inbox.size()));
    int row = 0;
    QSet<QString> retained;
    QDate previousDay;
    for (auto it = inbox.rbegin(); it != inbox.rend(); ++it, ++row) {
        auto *item = model_->item(row);
        if (!item) {
            auto owned = std::make_unique<QStandardItem>();
            item = owned.get();
            model_->setItem(row, owned.release());
        }
        auto key = eventKey(*it);
        retained.insert(key);
        auto id = std::visit([](const auto &e) { return e.auctionId.value(); }, *it);
        const auto *a = c_.store.get(id);
        const auto timestamp = std::visit([](const auto &e) { return e.timestamp; }, *it);
        const bool read = read_[c_.session.id().value()].contains(key);
        const auto day =
            QDateTime::fromMSecsSinceEpoch(
                std::chrono::duration_cast<common::Millis>(timestamp.time_since_epoch()).count())
                .date();
        const auto today =
            QDateTime::fromMSecsSinceEpoch(std::chrono::duration_cast<common::Millis>(
                                               c_.ticker.clock().wallNow().time_since_epoch())
                                               .count())
                .date();
        const QString heading = day == previousDay
                                    ? QString()
                                    : (day == today               ? QString("HÔM NAY")
                                       : day == today.addDays(-1) ? QString("HÔM QUA")
                                                                  : day.toString("dd/MM/yyyy")) +
                                          "\n";
        previousDay = day;
        item->setText(heading + (read ? QString() : QString("Chưa đọc · ")) +
                      Formatters::formatWallTime(timestamp) + "\n" +
                      (a ? QString::fromStdString(a->productName) + "\n" : QString()) +
                      c_.store.eventText(*it));
        item->setData(QVariant::fromValue<qulonglong>(id));
        item->setData(key, Qt::UserRole + 2);
        item->setForeground(QColor(read ? c_.theme.theme().textMuted : c_.theme.theme().text));
        QString icon = "bell";
        if (std::holds_alternative<domain::OutbidEvent>(*it))
            icon = "trending-up";
        else if (std::holds_alternative<domain::AuctionClosedEvent>(*it))
            icon = "award";
        else if (std::holds_alternative<domain::AuctionCancelledEvent>(*it))
            icon = "x";
        else if (std::holds_alternative<domain::CountdownEvent>(*it))
            icon = "clock";
        auto pixmap = IconProvider::icon(icon, QColor(c_.theme.theme().accent), 24)
                          .pixmap(QSize(24, 24), devicePixelRatioF());
        if (!read) {
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(c_.theme.theme().accent));
            painter.drawEllipse(QRectF(17, 0, 6, 6));
        }
        item->setIcon(QIcon(pixmap));
        item->setToolTip(item->text());
    }
    read_[c_.session.id().value()].intersect(retained);
    list_->setVisible(!inbox.empty());
    empty_->setVisible(inbox.empty());
    emit unreadChanged(unread());
}
int NotificationsPage::unread() const {
    int count = 0;
    for (const auto &e : c_.store.inbox())
        if (!read_.value(c_.session.id().value()).contains(eventKey(e)))
            ++count;
    return count;
}
void NotificationsPage::markAllRead() {
    for (const auto &e : c_.store.inbox())
        read_[c_.session.id().value()].insert(eventKey(e));
    refresh();
}
QStringList NotificationsPage::latest() const {
    QStringList result;
    for (int i = 0; i < std::min(5, model_->rowCount()); ++i)
        result << model_->item(i)->text();
    return result;
}
void NotificationsPage::openLatest(int index) {
    auto *item = model_->item(index);
    if (!item)
        return;
    read_[c_.session.id().value()].insert(item->data(Qt::UserRole + 2).toString());
    const auto id = item->data().toULongLong();
    refresh();
    emit openRequested(id);
}
class LogModel : public QAbstractListModel {
public:
    struct Entry {
        QString time, level, message;
    };
    LogModel(ThemeManager &theme, QObject *parent) : QAbstractListModel(parent), theme_(theme) {
        connect(&theme, &ThemeManager::changed, this, [this] {
            if (rowCount())
                emit dataChanged(index(0), index(rowCount() - 1));
        });
    }
    int rowCount(const QModelIndex &p = {}) const override {
        return p.isValid() ? 0 : static_cast<int>(entries_.size());
    }
    QVariant data(const QModelIndex &i, int role) const override {
        if (!i.isValid())
            return {};
        const auto &e = entries_[static_cast<size_t>(i.row())];
        if (role == Qt::DisplayRole)
            return e.time + "  [" + e.level + "]  " + e.message;
        if (role == Qt::UserRole)
            return e.level;
        if (role == Qt::ForegroundRole)
            return QColor(e.level == "ERROR"   ? theme_.theme().dangerText
                          : e.level == "WARN"  ? theme_.theme().warningText
                          : e.level == "DEBUG" ? theme_.theme().textMuted
                                               : theme_.theme().text);
        if (role == Qt::FontRole)
            return QFont("Consolas", 10);
        return {};
    }
    void append(Entry entry) {
        if (entries_.size() >= 2000) {
            beginRemoveRows({}, 0, 0);
            entries_.pop_front();
            endRemoveRows();
        }
        int row = rowCount();
        beginInsertRows({}, row, row);
        entries_.push_back(std::move(entry));
        endInsertRows();
    }

private:
    ThemeManager &theme_;
    std::deque<Entry> entries_;
};
class LogFilter : public QSortFilterProxyModel {
public:
    explicit LogFilter(QObject *parent) : QSortFilterProxyModel(parent) {}
    QSet<QString> levels{"DEBUG", "INFO", "WARN", "ERROR"};
    QString search;
    void refilter() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange();
#else
        invalidateFilter();
#endif
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override {
        auto index = sourceModel()->index(row, 0, parent);
        return levels.contains(index.data(Qt::UserRole).toString()) &&
               index.data().toString().contains(search, Qt::CaseInsensitive);
    }
};
SystemLogPage::SystemLogPage(UiContext &c, QWidget *parent) : QWidget(parent), c_(c) {
    setObjectName("systemLogPage");
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(24, 24, 24, 24);
    main->setSpacing(16);
    main->addWidget(label("Nhật ký hệ thống", this, 28));
    main->addWidget(
        label("2.000 dòng gần nhất · Nhật ký chi tiết vẫn được ghi vào tệp.", this, 14, "muted"));
    auto *row = new QHBoxLayout;
    model_ = new LogModel(c.theme, this);
    proxy_ = new LogFilter(this);
    proxy_->setSourceModel(model_);
    for (const QString level : {"DEBUG", "INFO", "WARN", "ERROR"}) {
        auto *chip = new Chip(level, this);
        chip->setChecked(true);
        row->addWidget(chip);
        connect(chip, &QPushButton::toggled, this, [this, level](bool on) {
            if (on)
                proxy_->levels.insert(level);
            else
                proxy_->levels.remove(level);
            proxy_->refilter();
            empty_->setVisible(proxy_->rowCount() == 0);
        });
    }
    auto *search = new QLineEdit(this);
    search->setObjectName("logSearch");
    search->setPlaceholderText("Tìm trong nhật ký…");
    row->addWidget(search, 1);
    connect(search, &QLineEdit::textChanged, this, [this](const QString &value) {
        proxy_->search = value;
        proxy_->refilter();
        empty_->setVisible(proxy_->rowCount() == 0);
    });
    autoscroll_ = new QCheckBox("Tự cuộn", this);
    autoscroll_->setChecked(true);
    row->addWidget(autoscroll_);
    auto *copy = button("Sao chép", "copyLog", this);
    row->addWidget(copy);
    connect(copy, &QPushButton::clicked, this, [this] {
        QStringList lines;
        for (int i = 0; i < proxy_->rowCount(); ++i)
            lines << proxy_->index(i, 0).data().toString();
        qApp->clipboard()->setText(lines.join('\n'));
    });
    main->addLayout(row);
    list_ = new QListView(this);
    list_->setObjectName("logList");
    list_->setModel(proxy_);
    list_->setWordWrap(true);
    list_->setResizeMode(QListView::Adjust);
    list_->setTextElideMode(Qt::ElideNone);
    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    main->addWidget(list_, 1);
    empty_ = new EmptyState("Chưa có nhật ký phù hợp",
                            "Nhật ký mới xuất hiện khi bạn thao tác hoặc phiên đổi trạng thái.", "",
                            c.theme, this, "log");
    main->addWidget(empty_);
    list_->setVisible(false);
}
void SystemLogPage::appendLog(const QString &level, const QString &message) {
    model_->append({Formatters::formatWallTime(c_.ticker.clock().wallNow()), level, message});
    empty_->setVisible(proxy_->rowCount() == 0);
    list_->setVisible(proxy_->rowCount() > 0);
    if (autoscroll_->isChecked())
        list_->scrollToBottom();
}
} // namespace auction::presentation
