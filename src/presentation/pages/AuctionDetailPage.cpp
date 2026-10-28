#include "auction/presentation/pages/AuctionDetailPage.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/util/SoundManager.h"
#include "auction/presentation/util/UiHelpers.h"
#include "auction/presentation/widgets/StateLifecycleWidget.h"

#include <QAbstractListModel>
#include <QDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QPainter>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <limits>
#include <memory>
namespace auction::presentation {
class BidHistoryModel : public QAbstractListModel {
public:
    BidHistoryModel(UiContext &c, QObject *parent) : QAbstractListModel(parent), c_(c) {
        connect(
            &c_.ticker, &UiTicker::tick, this,
            [this](common::WallTime wall, common::SteadyTime now) {
                auto second =
                    std::chrono::duration_cast<common::Seconds>(wall.time_since_epoch()).count();
                if (second != second_ && !bids_.empty()) {
                    second_ = second;
                    emit dataChanged(index(0), index(rowCount() - 1));
                }
                if (newRows_ > 0) {
                    emit dataChanged(index(0), index(newRows_ - 1));
                    if (now - flashAt_ >= common::Millis(600))
                        newRows_ = 0;
                }
            });
    }
    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : static_cast<int>(bids_.size());
    }
    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid())
            return {};
        const auto i = bids_.size() - 1 - static_cast<size_t>(index.row());
        const auto &b = bids_[i];
        if (role == Qt::DisplayRole)
            return c_.store.userName(b.bidderId());
        if (role == Qt::UserRole)
            return Formatters::formatMoney(b.amount());
        if (role == Qt::UserRole + 1) {
            auto seconds = std::max<int64_t>(0, std::chrono::duration_cast<common::Seconds>(
                                                    c_.ticker.clock().wallNow() - b.timestamp())
                                                    .count());
            return seconds < 5    ? QString("vừa xong")
                   : seconds < 60 ? QString::number(seconds) + " giây trước"
                                  : QString::number(seconds / 60) + " phút trước";
        }
        if (role == Qt::UserRole + 2)
            return "+" + Formatters::formatMoney(domain::Money(
                             b.amount().amount() - (i ? bids_[i - 1].amount().amount() : start_)));
        if (role == Qt::UserRole + 3)
            return QVariant::fromValue<qulonglong>(b.bidderId().value());
        if (role == Qt::UserRole + 4)
            return index.row() < newRows_
                       ? std::clamp(1.0 - std::chrono::duration_cast<common::Millis>(
                                              c_.ticker.clock().now() - flashAt_)
                                                  .count() /
                                              600.0,
                                    0.0, 1.0)
                       : 0.0;
        if (role == Qt::ToolTipRole)
            return c_.store.userName(b.bidderId()) + " · " + Formatters::formatMoney(b.amount()) +
                   " · " + Formatters::formatWallTime(b.timestamp());
        return {};
    }
    void refresh(const application::AuctionDetailDto &a) {
        if (id_ != a.id.value() || bids_.size() > a.bidHistory.size()) {
            beginResetModel();
            bids_ = a.bidHistory;
            id_ = a.id.value();
            start_ = a.startingPrice.amount();
            newRows_ = 0;
            endResetModel();
            return;
        }
        const int added = static_cast<int>(a.bidHistory.size() - bids_.size());
        if (added) {
            beginInsertRows({}, 0, added - 1);
            bids_ = a.bidHistory;
            newRows_ = added;
            flashAt_ = c_.ticker.clock().now();
            endInsertRows();
            if (rowCount() > added)
                emit dataChanged(index(added), index(added));
        }
    }

private:
    UiContext &c_;
    quint64 id_{};
    int64_t start_{}, second_{};
    int newRows_{};
    common::SteadyTime flashAt_{};
    std::vector<domain::LuotTraGia> bids_;
};
class BidDelegate : public QStyledItemDelegate {
public:
    BidDelegate(ThemeManager &theme, QObject *parent)
        : QStyledItemDelegate(parent), theme_(theme) {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return {360, 56};
    }
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &i) const override {
        const auto &t = theme_.theme();
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->fillRect(opt.rect, QColor(i.row() == 0 ? t.bgElevated : t.bgSurface));
        const auto flash = i.data(Qt::UserRole + 4).toDouble();
        if (flash > 0) {
            auto color = QColor(t.success);
            color.setAlphaF(flash * 0.12);
            p->fillRect(opt.rect, color);
            p->setClipRect(opt.rect);
            p->translate(qRound(12 * flash), 0);
        }
        QRect r = opt.rect.adjusted(8, 7, -8, -7);
        QRect avatar(r.left(), r.top() + 3, 32, 32);
        auto colors = Theme::avatarColors();
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(colors[static_cast<int>(i.data(Qt::UserRole + 3).toULongLong() % 8)]));
        p->drawEllipse(avatar);
        p->setPen(Qt::white);
        p->setFont(Fonts::body(12, QFont::DemiBold));
        p->drawText(avatar, Qt::AlignCenter, i.data().toString().section(' ', -1).left(1));
        const int moneyWidth = qMin(220, r.width() / 2);
        QRect nameRect(r.left() + 44, r.top(), r.width() - moneyWidth - 48, 20);
        p->setPen(QColor(t.text));
        p->setFont(Fonts::body(14, QFont::DemiBold));
        p->drawText(
            nameRect, Qt::AlignLeft,
            p->fontMetrics().elidedText(i.data().toString(), Qt::ElideRight, nameRect.width()));
        p->setFont(Fonts::numbers(14, QFont::DemiBold));
        p->setPen(QColor(i.row() == 0 ? t.accent : t.text));
        p->drawText(QRect(r.right() - moneyWidth, r.top(), moneyWidth, 20), Qt::AlignRight,
                    i.data(Qt::UserRole).toString());
        p->setFont(Fonts::body(12));
        p->setPen(QColor(t.textMuted));
        p->drawText(QRect(r.left() + 44, r.top() + 23, r.width() - 44, 18), Qt::AlignLeft,
                    i.data(Qt::UserRole + 1).toString() + (i.row() == 0 ? " · Dẫn đầu" : ""));
        p->drawText(QRect(r.right() - moneyWidth, r.top() + 23, moneyWidth, 18), Qt::AlignRight,
                    i.data(Qt::UserRole + 2).toString());
        p->restore();
    }

private:
    ThemeManager &theme_;
};
AuctionDetailPage::AuctionDetailPage(UiContext &c, QWidget *parent) : QScrollArea(parent), c_(c) {
    setObjectName("auctionDetailPage");
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    body_ = new QWidget(this);
    body_->setObjectName("pageBody");
    auto *main = new QVBoxLayout(body_);
    main->setContentsMargins(24, 16, 24, 24);
    main->setSpacing(12);
    setWidget(body_);
    auto *back = button("← Sàn đấu giá", "detailBack", body_, "ghost");
    main->addWidget(back, 0, Qt::AlignLeft);
    connect(back, &QPushButton::clicked, this, &AuctionDetailPage::backRequested);
    loading_=label("Đang tải…",body_,14,"muted");loading_->setObjectName("lblDetailLoading");main->addWidget(loading_);
    columns_ = new QBoxLayout(QBoxLayout::LeftToRight);
    columns_->setSpacing(24);
    main->addLayout(columns_);
    main->addStretch();
    left_ = new QWidget(body_);
    auto *left = new QVBoxLayout(left_);
    left->setContentsMargins(0, 0, 0, 0);
    left->setSpacing(12);
    image_ = new ProductImageView(c.images, left_);
    image_->setMinimumHeight(350);
    image_->setMaximumHeight(480);
    image_->setCursor(Qt::PointingHandCursor);
    image_->setToolTip("Nhấp chuột để xem ảnh phóng to kích thước lớn");
    left->addWidget(image_);

    connect(image_, &ProductImageView::clicked, this, [this] {
        const auto *a = c_.store.get(id_);
        const auto title = a ? QString::fromStdString(a->productName) : "Vật phẩm";
        const auto category = a ? QString::fromStdString(a->category) : "";
        auto *dialog = new QDialog(this);
        dialog->setObjectName("imageLightboxDialog");
        dialog->setWindowTitle("Ảnh chi tiết: " + title);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->resize(960, 720);
        auto *layout = new QVBoxLayout(dialog);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);

        auto *header = new QHBoxLayout;
        auto *titleLabel = label(title, dialog, 16);
        titleLabel->setFont(Fonts::body(16, QFont::Bold));
        header->addWidget(titleLabel);
        header->addStretch();

        auto *btnClose = button("Đóng", "btnCloseLightbox", dialog, "primary");
        btnClose->setFixedWidth(100);
        connect(btnClose, &QPushButton::clicked, dialog, &QDialog::accept);
        header->addWidget(btnClose);
        layout->addLayout(header);

        auto *preview = new ProductImageView(c_.images, dialog);
        preview->setProduct(id_, category);
        preview->setMinimumHeight(450);
        preview->setMaximumHeight(16777215);
        layout->addWidget(preview, 1);

        dialog->open();
    });
    title_ = new ElidedLabel(left_, 28);
    left->addWidget(title_);
    category_ = new ElidedLabel(left_, 12);
    tone(category_, "muted");
    left->addWidget(category_);
    lifecycleWidget_ = new StateLifecycleWidget(c.theme, left_);
    left->addWidget(lifecycleWidget_);
    auto *tabs = new QTabWidget(left_);
    auto *historyPage = new QWidget(tabs);
    auto *historyLayout = new QVBoxLayout(historyPage);
    historyLayout->setContentsMargins(0, 12, 0, 0);
    history_ = new QListView(historyPage);
    history_->setObjectName("tblBidHistory");
    historyModel_ = new BidHistoryModel(c, history_);
    history_->setModel(historyModel_);
    history_->setItemDelegate(new BidDelegate(c.theme, history_));
    history_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    history_->setMinimumHeight(336);
    history_->setSelectionMode(QAbstractItemView::NoSelection);
    historyLayout->addWidget(history_);
    historyEmpty_ =
        label("Chưa có lượt đặt giá nào. Hãy là người đầu tiên!", historyPage, 14, "muted");
    historyLayout->addWidget(historyEmpty_);
    tabs->addTab(historyPage, "Lịch sử đặt giá");
    description_ = label("", tabs);
    description_->setAlignment(Qt::AlignTop);
    description_->setMargin(16);
    tabs->addTab(description_, "Mô tả");
    participants_ = new QListView(tabs);
    participants_->setObjectName("participantsList");
    participantsModel_ = new QStandardItemModel(participants_);
    participants_->setModel(participantsModel_);
    participants_->setIconSize(QSize(32, 32));
    participants_->setWordWrap(true);
    participants_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    participants_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabs->addTab(participants_, "Người tham gia");
    left->addWidget(tabs);
    left->addStretch();
    auto *rightCard = new Card(body_);
    right_ = rightCard;
    rightCard->content->setContentsMargins(18, 18, 18, 18);
    rightCard->content->setSpacing(10);
    auto *right = rightCard->content;
    auto *first = new QHBoxLayout;
    badge_ = new StateBadge(c.theme, right_);
    follow_ = button("Theo dõi", "btnFollow", right_, "ghost");
    follow_->setProperty("size", "sm");
    first->addWidget(badge_);
    first->addStretch();
    first->addWidget(follow_);
    right->addLayout(first);
    connect(follow_, &QPushButton::clicked, this, [this] { emit followRequested(id_); });
    timeCaption_ = label("Kết thúc sau", right_, 12, "muted");
    right->addWidget(timeCaption_);
    countdown_ = new CountdownLabel(c.ticker, right_, true);
    right->addWidget(countdown_);
    auctioneerCallout_ = label("", right_, 13);
    auctioneerCallout_->setObjectName("auctioneerCallout");
    auctioneerCallout_->setAlignment(Qt::AlignCenter);
    auctioneerCallout_->setWordWrap(true);
    auctioneerCallout_->hide();
    right->addWidget(auctioneerCallout_);
    gavelBanner_ = label("", right_, 13);
    gavelBanner_->setObjectName("gavelBanner");
    gavelBanner_->setAlignment(Qt::AlignCenter);
    gavelBanner_->setWordWrap(true);
    gavelBanner_->hide();
    right->addWidget(gavelBanner_);
    progress_ = new QProgressBar(right_);
    progress_->setRange(0, 1000);
    progress_->setTextVisible(false);
    progress_->setAccessibleName("Tiến độ phiên");
    right->addWidget(progress_);
    right->addWidget(label("GIÁ HIỆN TẠI", right_, 12, "muted"));
    price_ = new PriceLabel(c.theme, right_);
    right->addWidget(price_);
    leader_ = new ElidedLabel(right_, 14);
    tone(leader_, "muted");
    leader_->setObjectName("lblLeader");
    right->addWidget(leader_);
    auto *params = new QGridLayout;
    params->setHorizontalSpacing(16);
    params->setVerticalSpacing(12);
    QStringList labels = {"Giá khởi điểm", "Bước giá", "Giá tối thiểu tiếp theo",
                          "Ký quỹ (đặt cọc)"};
    for (int i = 0; i < 4; ++i) {
        auto *cell = new QWidget(right_);
        auto *l = new QVBoxLayout(cell);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(4);
        l->addWidget(label(labels[i], cell, 12, "muted"));
        auto *v = label("", cell, 16);
        v->setFont(Fonts::numbers(16, QFont::DemiBold));
        l->addWidget(v);
        params_ << v;
        params->addWidget(cell, i / 2, i % 2);
    }
    right->addLayout(params);
    myStatus_ = label("", right_, 14);
    myStatus_->setObjectName("lblMyStatus");
    right->addWidget(myStatus_);
    steps_ = new StepIndicator({"Đặt cọc", "Đặt giá", "Thanh toán"}, right_);
    right->addWidget(steps_);
    join_ = button("Tham gia đấu giá", "btnJoin", right_, "primary");
    right->addWidget(join_);
    connect(join_, &QPushButton::clicked, this, [this] { emit joinRequested(id_); });
    bidControls_ = new QWidget(right_);
    auto *controls = new QVBoxLayout(bidControls_);
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(8);
    auto *moneyRow = new QHBoxLayout;
    auto *minus = button("−", "btnBidMinus", bidControls_);
    minus->setAccessibleName("Giảm một bước giá");
    amount_ = new MoneyLineEdit(bidControls_);
    amount_->setObjectName("edtBidAmount");
    auto *plus = button("+", "btnBidPlus", bidControls_);
    plus->setAccessibleName("Tăng một bước giá");
    moneyRow->addWidget(minus);
    moneyRow->addWidget(amount_, 1);
    moneyRow->addWidget(plus);
    controls->addLayout(moneyRow);
    auto adjust = [this](int steps) {
        const auto *a = c_.store.get(id_);
        if (!a)
            return;
        const auto step = a->minIncrement.amount();
        const auto current = amount_->value();
        if (steps > 0 && current > std::numeric_limits<qint64>::max() - step)
            return;
        amount_->setValue(std::max<int64_t>(0, current + steps * step));
        amountDirty_ = true;
    };
    connect(minus, &QPushButton::clicked, this, [adjust] { adjust(-1); });
    connect(plus, &QPushButton::clicked, this, [adjust] { adjust(1); });
    auto *suggestions = new QHBoxLayout;
    for (int i : {0, 1, 2, 5}) {
        auto *chip =
            new Chip(i == 0 ? "Tối thiểu" : "+" + QString::number(i) + " bước", bidControls_);
        chip->setObjectName(i == 0 ? "btnBidMin" : "bidSuggestion" + QString::number(i));
        suggestions->addWidget(chip);
        connect(chip, &QPushButton::clicked, this, [this, i] {
            const auto *a = c_.store.get(id_);
            if (!a)
                return;
            const auto base = a->minimumNextBid.amount();
            const auto step = a->minIncrement.amount();
            if (i && step > (std::numeric_limits<qint64>::max() - base) / i)
                return;
            amount_->setValue(base + i * step);
            amountDirty_ = true;
            setError({});
        });
    }
    controls->addLayout(suggestions);
    bid_ = button("Đặt giá", "btnPlaceBid", bidControls_, "primary");
    controls->addWidget(bid_);
    connect(bid_, &QPushButton::clicked, this, &AuctionDetailPage::submitBid);
    connect(amount_, &QLineEdit::returnPressed, this, &AuctionDetailPage::submitBid);
    connect(amount_, &QLineEdit::textEdited, this, [this] { amountDirty_ = true; });
    connect(amount_, &MoneyLineEdit::valueChanged, this, [this](qint64 value) {
        bid_->setText("Đặt giá " + Formatters::formatMoney(domain::Money(value)));
    });
    right->addWidget(bidControls_);
    error_ = label("", right_, 12, "danger");
    error_->setObjectName("lblBidError");
    right->addWidget(error_);
    fix_ = button("", "btnBidFix", right_, "ghost");
    right->addWidget(fix_);
    fix_->hide();
    connect(fix_, &QPushButton::clicked, this, [this] {
        if (fix_->text() == "Nạp tiền")
            emit topUpRequested();
        else if (fix_->text() == "Tham gia")
            emit joinRequested(id_);
        else {
            if (auto *a = c_.store.get(id_))
                amount_->setValue(a->minimumNextBid.amount());
            setError({});
        }
    });
    ownerControls_ = new QWidget(right_);
    ownerControls_->setObjectName("ownerControls");
    auto *owner = new QVBoxLayout(ownerControls_);
    owner->setContentsMargins(0, 0, 0, 0);
    owner->addWidget(label("Bạn là người bán", ownerControls_, 18));
    closeEarly_ = button("Chốt phiên", "btnCloseEarly", ownerControls_, "danger");
    owner->addWidget(closeEarly_);
    closeReason_ = label("", ownerControls_, 14, "muted");
    closeReason_->setObjectName("closeEarlyReason");
    owner->addWidget(closeReason_);
    connect(closeEarly_, &QPushButton::clicked, this, [this] { emit closeEarlyRequested(id_); });
    right->addWidget(ownerControls_);
    earlyBadge_ = label("Chốt sớm", right_, 14, "accent");
    earlyBadge_->setObjectName("closedEarlyBadge");
    right->addWidget(earlyBadge_);
    result_ = label("", right_, 14);
    result_->setObjectName("auctionResult");
    right->addWidget(result_);
    auto *management = new QHBoxLayout;
    approve_ = button("Duyệt phiên", "btnApprove", right_);
    cancel_ = button("Hủy phiên", "btnCancelAuction", right_, "danger");
    deleteAuctionButton_ = button("🗑️ Xóa phiên", "btnDeleteAuction", right_, "ghost");
    deleteAuctionButton_->setToolTip("Xóa vĩnh viễn phiên đấu giá và đồng bộ thời gian thực");
    management->addWidget(approve_);
    management->addWidget(cancel_);
    management->addWidget(deleteAuctionButton_);
    right->addLayout(management);
    connect(approve_, &QPushButton::clicked, this, [this] { emit approveRequested(id_); });
    connect(cancel_, &QPushButton::clicked, this, [this] { emit cancelRequested(id_); });
    connect(deleteAuctionButton_, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, "Xác nhận xóa",
                                  "Bạn có chắc chắn muốn xóa vĩnh viễn phiên đấu giá này không?\n"
                                  "Dữ liệu phiên sẽ được xóa sạch khỏi hệ thống và đồng bộ trên tất cả máy khách.",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            emit deleteRequested(id_);
        }
    });
    columns_->addWidget(left_, 6);
    columns_->addWidget(right_, 4);
    columns_->setAlignment(right_, Qt::AlignTop);
    connect(&c.store, &AuctionStore::auctionChanged, this, [this](quint64 id) {
        if (id == id_)
            refresh();
    });
    connect(&c.session, &UserSession::changed, this, [this] {
        amountDirty_ = false;
        setError({});
        refresh();
    });
    connect(&c.ticker, &UiTicker::tick, this, [this] { refreshTime(); });
}
void AuctionDetailPage::openAuction(quint64 id) {
    id_ = id;
    lastCountdownSec_ = -1;
    lastGavelAuctionId_ = 0;
    amountDirty_ = false;
    setError({});
    refresh();
    updateColumns();
    verticalScrollBar()->setValue(0);
}
void AuctionDetailPage::refresh() {
    const auto *a = c_.store.get(id_);
    if (!a) {
        loading_->show();left_->hide();right_->hide();
        return;
    }
    loading_->hide();left_->show();right_->show();
    title_->setText(QString::fromStdString(a->productName));
    title_->setToolTip(title_->text());
    category_->setText(QString::fromStdString(a->category) + "  ·  Phiên #" + QString::number(id_) +
                       "  ·  Người bán: " + QString::fromStdString(a->sellerName));
    image_->setProduct(id_, QString::fromStdString(a->category));
    category_->setToolTip(category_->text());
    description_->setText(QString::fromStdString(a->productDescription));
    participantsModel_->setRowCount(std::max(1, static_cast<int>(a->participants.size())));
    int participantRow = 0;
    for (auto id : a->participants) {
        auto *item = participantsModel_->item(participantRow);
        if (!item) {
            auto owned = std::make_unique<QStandardItem>();
            item = owned.get();
            participantsModel_->setItem(participantRow, owned.release());
        }
        const auto name = c_.store.userName(id);
        item->setText(name + (id == c_.session.id() ? " (Bạn)" : "") + "\nĐã đặt cọc");
        item->setToolTip(item->text());
        const auto dpr = devicePixelRatioF();
        QPixmap avatar(qRound(32 * dpr), qRound(32 * dpr));
        avatar.setDevicePixelRatio(dpr);
        avatar.fill(Qt::transparent);
        QPainter painter(&avatar);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(Theme::avatarColors()[static_cast<int>(id.value() % 8)]));
        painter.drawEllipse(QRectF(0, 0, 32, 32));
        painter.setPen(Qt::white);
        painter.setFont(Fonts::body(12, QFont::DemiBold));
        painter.drawText(QRect(0, 0, 32, 32), Qt::AlignCenter, name.section(' ', -1).left(1));
        painter.end();
        item->setIcon(QIcon(avatar));
        ++participantRow;
    }
    if (a->participants.empty()) {
        auto item = std::make_unique<QStandardItem>("Chưa có người tham gia phiên này.");
        participantsModel_->setItem(0, item.release());
    }
    historyModel_->refresh(*a);
    historyEmpty_->setVisible(a->bidHistory.empty());
    history_->setVisible(!a->bidHistory.empty());
    badge_->setState(QString::fromStdString(a->state));
    lifecycleWidget_->setState(QString::fromStdString(a->state));
    auto end = c_.store.endSteady(id_);
    countdown_->setDeadline(end,
                            end - std::chrono::duration_cast<common::SteadyTime::duration>(
                                      a->durationMs),
                            QString::fromStdString(a->state));
    const auto status = myStatus(*a, c_.session);
    myStatus_->setText(status);
    tone(myStatus_, status.contains("vượt")                                  ? "danger"
                    : status.contains("dẫn đầu") || status.contains("thắng") ? "success"
                                                                             : "muted");
    price_->setAmount(a->currentPrice, status.contains("vượt"));
    leader_->setText(a->leaderId ? "Người dẫn đầu: " + QString::fromStdString(a->leaderName)
                                 : "Chưa có ai đặt giá");
    leader_->setToolTip(leader_->text());
    const domain::Money values[] = {a->startingPrice, a->minIncrement, a->minimumNextBid,
                                    a->depositAmount};
    for (int i = 0; i < 4; ++i)
        params_[i]->setText(Formatters::formatMoney(values[i]));
    const bool owns = c_.session.loggedIn() && a->sellerId == c_.session.id();
    const bool canClose = owns && isLive(*a) && !a->bidHistory.empty() && c_.store.canWrite(id_);
    ownerControls_->setVisible(owns && !isClosed(*a));
    closeEarly_->setEnabled(canClose);
    closeEarly_->setText("Chốt phiên · bán với giá " + Formatters::formatMoney(a->currentPrice));
    const QString whyClose = !isLive(*a) ? "Phiên chưa diễn ra" :
                             a->bidHistory.empty() ? "Chưa có lượt đặt giá nào" : "";
    closeReason_->setText(whyClose);
    closeReason_->setVisible(!whyClose.isEmpty());
    closeEarly_->setToolTip(canClose ? "Kết thúc ngay và quyết toán theo giá đang dẫn đầu" : whyClose);
    earlyBadge_->setVisible(isClosed(*a) && a->closedEarly);
    steps_->setVisible(!owns && !isClosed(*a));
    myStatus_->setVisible(!owns || isClosed(*a));
    bool joined = hasJoined(*a, c_.session.id());
    bool eligible = c_.session.loggedIn() && a->sellerId != c_.session.id() && !isClosed(*a) &&
                    a->state != "Chờ duyệt";
    join_->setVisible(!joined && !isClosed(*a) && a->sellerId != c_.session.id());
    join_->setEnabled(eligible && c_.store.canWrite(id_));
    join_->setText("Tham gia · đặt cọc " + Formatters::formatMoney(a->depositAmount));
    join_->setToolTip(eligible ? "Xác nhận ký quỹ trước khi tham gia" : status);
    bidControls_->setVisible(joined && !isClosed(*a) && c_.session.loggedIn());
    if (!amountDirty_)
        amount_->setValue(a->minimumNextBid.amount());
    follow_->setVisible(c_.session.loggedIn() && a->sellerId != c_.session.id() && !isClosed(*a));
    follow_->setEnabled(!isClosed(*a) && c_.store.canWrite(id_));
    follow_->setText(c_.session.follows(id_) ? "Bỏ theo dõi" : "Theo dõi");
    follow_->setToolTip(isClosed(*a) ? "Phiên đã đóng" : "Nhận thông báo về phiên này");
    approve_->setVisible((c_.session.loggedIn() && a->sellerId == c_.session.id()) && a->state == "Chờ duyệt");
    cancel_->setVisible(
        (c_.session.loggedIn() && a->sellerId == c_.session.id()) &&
        !isClosed(*a));
    const bool canDelete = c_.session.loggedIn() && owns;
    deleteAuctionButton_->setVisible(canDelete);
    approve_->setEnabled(c_.store.canWrite(id_));cancel_->setEnabled(c_.store.canWrite(id_));deleteAuctionButton_->setEnabled(c_.store.canWrite(id_));
    if(!c_.store.canWrite(id_))for(auto *b:{join_,follow_,approve_,cancel_,closeEarly_,deleteAuctionButton_})b->setToolTip("Mất kết nối hoặc đang chờ máy chủ");
    steps_->setStep(isClosed(*a) ? 2 : joined ? 1 : 0);
    result_->setVisible(isClosed(*a));
    if (isClosed(*a)) {
        auctioneerCallout_->hide();
        gavelBanner_->show();
        const bool sold = a->outcome == domain::AuctionOutcome::Sold;
        const bool wonByMe = (c_.session.loggedIn() && a->leaderId == c_.session.id());
        if (sold) {
            gavelBanner_->setText(QString("🔨 CẠCH! GÕ BÚA CHỐT PHIÊN!\nLẦN THỨ BA... BÁN THÀNH CÔNG!\n🏆 %1 %2")
                .arg(wonByMe ? "CHÚC MỪNG BẠN ĐÃ THẮNG PHIÊN!" : "Người thắng: " + QString::fromStdString(a->leaderName))
                .arg("với giá " + Formatters::formatMoney(a->currentPrice)));
            gavelBanner_->setStyleSheet(
                "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #15803d, stop:1 #166534); "
                "color: white; font-weight: bold; padding: 12px; border-radius: 10px; border: 2px solid #4ade80; font-size: 13px;");
            if (lastGavelAuctionId_ != id_) {
                lastGavelAuctionId_ = id_;
                SoundManager::instance().playGavel();
                if (wonByMe) {
                    SoundManager::instance().playVictory();
                }
            }
        } else {
            gavelBanner_->setText("🔨 CẠCH! GÕ BÚA CHỐT PHIÊN!\nLẦN THỨ BA... KHÔNG CÓ LƯỢT ĐẶT GIÁ HỢP LỆ!");
            gavelBanner_->setStyleSheet(
                "background: rgba(127, 127, 127, 0.15); color: #94a3b8; font-weight: bold; padding: 10px; border-radius: 10px; border: 1px dashed #64748b; font-size: 13px;");
            if (lastGavelAuctionId_ != id_) {
                lastGavelAuctionId_ = id_;
                SoundManager::instance().playGavel();
            }
        }
        QString text = a->outcome == domain::AuctionOutcome::Sold
            ? "Đã bán thành công với giá " + Formatters::formatMoney(a->currentPrice)
            : a->outcome == domain::AuctionOutcome::PaymentDefault
                ? "Người thắng không đủ tiền thanh toán; đã xử lý tịch thu cọc theo quy định."
                : Formatters::formatAuctionOutcome(a->outcome);
        const bool defaulted = a->outcome == domain::AuctionOutcome::PaymentDefault;
        if (sold || defaulted) {
            text += "\nNgười thắng: " + QString::fromStdString(a->leaderName) +
                    "\nGiá chốt: " + Formatters::formatMoney(a->currentPrice);
            if (sold)
                text += "\nCọc trừ vào giá: " + Formatters::formatMoney(a->depositAmount) +
                        "\nĐã thanh toán thêm: " +
                        Formatters::formatMoney(a->currentPrice - a->depositAmount);
        }
        const auto refunds = a->participants.size() - ((sold || defaulted) && a->leaderId ? 1 : 0);
        text += "\nHoàn cọc cho " + QString::number(refunds) + " người";
        if (joined) {
            if (a->leaderId == c_.session.id() && (sold || defaulted))
                text += "\nBạn đã trả: " +
                        Formatters::formatMoney(sold ? a->currentPrice : a->depositAmount);
            else
                text += "\nBạn được hoàn: " + Formatters::formatMoney(a->depositAmount);
        }
        result_->setText(text);
    } else {
        gavelBanner_->hide();
    }
    refreshTime();
}
void AuctionDetailPage::refreshTime() {
    const auto *a = c_.store.get(id_);
    if (!a)
        return;
    auto now = c_.ticker.clock().now();
    auto end = c_.store.endSteady(id_);
    auto duration = std::chrono::duration_cast<common::Millis>(a->durationMs);
    auto start = end - duration;
    const bool before = now < start;
    const bool pending = a->state == "Chờ duyệt";
    countdown_->setVisible(!pending);
    progress_->setVisible(!pending && !isClosed(*a));
    const auto caption = pending ? pendingSchedule(*a) : a->closedEarly ? QString("Đã kết thúc · Chốt sớm") : a->state == "Hủy" ? QString("Đã hủy · Tiền cọc được hoàn lại")
                         : isClosed(*a)
                             ? "Đã kết thúc lúc " + (a->endWallTime ? Formatters::formatWallTime(*a->endWallTime) : QString("—"))
                             : (before ? QString("Bắt đầu sau") : QString("Kết thúc sau"));
    if (timeCaption_->text() != caption)
        timeCaption_->setText(caption);
    progress_->setValue(duration.count() > 0
                            ? static_cast<int>(std::clamp<int64_t>(
                                  std::chrono::duration_cast<common::Millis>(now - start).count() *
                                      1000 / duration.count(),
                                  0, 1000))
                            : 0);
    tone(progress_, isClosed(*a)                       ? "muted"
                    : before                           ? "info"
                    : end - now <= common::Seconds(10) ? "danger"
                    : end - now <= common::Seconds(60) ? "warning"
                                                       : "accent");

    const auto remDuration = end - now;
    const auto remSeconds = std::chrono::duration_cast<std::chrono::seconds>(remDuration).count();
    if (isLive(*a) && remSeconds <= 10 && remSeconds > 0) {
        auctioneerCallout_->show();
        if (remSeconds > 5) {
            auctioneerCallout_->setText(QString("🔨 CHỦ TỌA PHIÊN: \"%1 LẦN THỨ NHẤT!\"").arg(Formatters::formatMoney(a->currentPrice)));
            auctioneerCallout_->setStyleSheet("background: #ea580c; color: white; font-weight: bold; padding: 8px 12px; border-radius: 8px; font-size: 13px;");
        } else {
            auctioneerCallout_->setText(QString("🔨 CHỦ TỌA PHIÊN: \"%1 LẦN THỨ HAI! AI TRẢ CAO HƠN KHÔNG?\"").arg(Formatters::formatMoney(a->currentPrice)));
            auctioneerCallout_->setStyleSheet("background: #dc2626; color: white; font-weight: bold; padding: 8px 12px; border-radius: 8px; font-size: 13px;");
        }
        countdown_->setStyleSheet(remSeconds % 2 == 0 ? "color: #ef4444; font-weight: bold; font-size: 26px;" : "color: #f97316; font-weight: bold; font-size: 26px;");
        if (lastCountdownSec_ != remSeconds) {
            lastCountdownSec_ = static_cast<int>(remSeconds);
            SoundManager::instance().playCountdownTick();
        }
    } else {
        auctioneerCallout_->hide();
        countdown_->setStyleSheet("");
        if (remSeconds > 10) {
            lastCountdownSec_ = -1;
        }
    }
    const bool allowed = c_.store.canWrite(id_) && c_.session.loggedIn() && hasJoined(*a, c_.session.id()) && isLive(*a) &&
                         now < end && a->leaderId != c_.session.id();
    bid_->setEnabled(allowed);
    amount_->setEnabled(allowed);
    if (!allowed) {
        const QString why =
            !c_.store.canWrite(id_)           ? "Mất kết nối hoặc đang chờ máy chủ."
            : isClosed(*a)                      ? "Phiên đã đóng, không thể đặt giá."
            : a->leaderId == c_.session.id()  ? "Bạn đang dẫn đầu. Chờ người khác đặt giá."
            : !hasJoined(*a, c_.session.id()) ? "Bạn cần đặt cọc để tham gia trước khi đặt giá."
            : !isLive(*a)                     ? "Phiên chưa mở, hãy chờ đến giờ bắt đầu."
                                              : "Phiên đã hết giờ.";
        bid_->setToolTip(why);
        if (error_->text().isEmpty()) {
            myStatus_->setToolTip(why);
            if (hasJoined(*a, c_.session.id()) && !isClosed(*a) && !allowed)
                myStatus_->setText(myStatus(*a, c_.session) + "\n" + why);
        }
    }
}
void AuctionDetailPage::submitBid() {
    if(!c_.store.canWrite(id_))return;
    const auto *a = c_.store.get(id_);
    if (!a)
        return;
    if (!hasJoined(*a, c_.session.id())) {
        setError("Bạn cần đặt cọc để tham gia trước khi đặt giá.", "Tham gia");
        return;
    }
    if (!c_.session.loggedIn() || !isLive(*a) || c_.ticker.clock().now() >= c_.store.endSteady(id_)) {
        setError("Phiên chưa mở hoặc đã đóng, không thể đặt giá.");
        return;
    }
    if (a->leaderId == c_.session.id()) {
        setError("Bạn đang dẫn đầu. Chờ người khác đặt giá.");
        return;
    }
    setError({});
    SoundManager::instance().playBidSuccess();
    emit bidRequested(id_, amount_->value());
}
void AuctionDetailPage::setError(const QString &error, const QString &action) {
    error_->setText(error);
    error_->setVisible(!error.isEmpty());
    fix_->setText(action);
    fix_->setVisible(!action.isEmpty());
}
void AuctionDetailPage::resizeEvent(QResizeEvent *event) {
    QScrollArea::resizeEvent(event);
    updateColumns();
}
void AuctionDetailPage::updateColumns() {
    const bool narrow = viewport()->width() - 48 < 1100;
    if (narrow == stacked_)
        return;
    stacked_ = narrow;
    columns_->removeWidget(left_);
    columns_->removeWidget(right_);
    columns_->setDirection(narrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    if (narrow) {
        columns_->addWidget(right_);
        columns_->addWidget(left_);
    } else {
        columns_->addWidget(left_, 6);
        columns_->addWidget(right_, 4);
    }
    columns_->setAlignment(right_, Qt::AlignTop);
}
} // namespace auction::presentation
