#include "auction/presentation/widgets/AuctionCard.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QMouseEvent>
#include <QPainter>
namespace auction::presentation {
ProductImageView::ProductImageView(ProductImageStore &images, QWidget *parent)
    : QWidget(parent), images_(images) {
    setObjectName("productImageView");
    auto policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    setMinimumHeight(100);
    setMaximumHeight(480);
    connect(&images, &ProductImageStore::changed, this, [this](quint64 id) {
        if (id == id_) {
            cached_ = {};
            update();
        }
    });
}
void ProductImageView::setProduct(quint64 id, const QString &category) {
    if (id_ != id || category_ != category) {
        id_ = id;
        category_ = category;
        cached_ = {};
        update();
    }
}
void ProductImageView::paintEvent(QPaintEvent *) {
    if (cached_.isNull() || cachedSize_ != size() || cachedDpr_ != devicePixelRatioF()) {
        cached_ = images_.image(id_, category_, size(), devicePixelRatioF());
        cachedSize_ = size();
        cachedDpr_ = devicePixelRatioF();
    }
    QPainter p(this);
    p.drawPixmap(0, 0, cached_);
}
void ProductImageView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
AuctionCard::AuctionCard(quint64 id, AuctionStore &store, UserSession &session, UiTicker &ticker,
                         ThemeManager &theme, ProductImageStore &images, QWidget *parent)
    : Card(parent), id_(id), store_(store), session_(session) {
    setObjectName("auctionCard_" + QString::number(id));
    setProperty("interactive", true);
    setCursor(Qt::PointingHandCursor);
    setMinimumWidth(280);
    content->setContentsMargins(16, 16, 16, 16);
    content->setSpacing(10);
    image_ = new ProductImageView(images, this);
    image_->setAttribute(Qt::WA_TransparentForMouseEvents);
    image_->setMaximumHeight(200);
    content->addWidget(image_);
    auto *row = new QHBoxLayout;
    badge_ = new StateBadge(theme, this);
    countdown_ = new CountdownLabel(ticker, this);
    pendingSchedule_ = label("", this, 12, "muted");
    pendingSchedule_->setObjectName("pendingSchedule");
    pendingSchedule_->setWordWrap(true);
    row->addWidget(badge_);
    row->addStretch();
    row->addWidget(countdown_);
    content->addLayout(row);
    content->addWidget(pendingSchedule_);
    title_ = new ElidedLabel(this, 18);
    content->addWidget(title_);
    category_ = label("", this, 12, "muted");
    content->addWidget(category_);
    caption_ = label("GIÁ HIỆN TẠI", this, 12, "muted");
    content->addWidget(caption_);
    price_ = new PriceLabel(theme, this, 24);
    content->addWidget(price_);
    counts_ = label("", this, 12, "muted");
    content->addWidget(counts_);
    status_ = label("", this, 12);
    content->addWidget(status_);
    open_ = button("Xem phiên", "cardOpen", this);
    content->addWidget(open_);
    connect(open_, &QPushButton::clicked, this, [this] { emit openRequested(id_); });
    auto *actions = new QHBoxLayout;
    approve_ = button("Duyệt", "cardApprove", this);
    cancel_ = button("Hủy phiên", "cardCancel", this, "danger");
    actions->addWidget(approve_);
    actions->addWidget(cancel_);
    content->addLayout(actions);
    approve_->hide();
    cancel_->hide();
    connect(approve_, &QPushButton::clicked, this, [this] { emit approveRequested(id_); });
    connect(cancel_, &QPushButton::clicked, this, [this] { emit cancelRequested(id_); });
    connect(&store, &AuctionStore::auctionChanged, this, [this](quint64 changed) {
        if (changed == id_)
            refresh();
    });
    connect(&session, &UserSession::changed, this, &AuctionCard::refresh);
    refresh();
}
void AuctionCard::refresh() {
    const auto *a = store_.get(id_);
    if (!a)
        return;
    image_->setProduct(id_, QString::fromStdString(a->category));
    badge_->setState(QString::fromStdString(a->state));
    auto end = store_.endSteady(id_);
    auto length = a->durationMs;
    countdown_->setDeadline(end,
                            end - std::chrono::duration_cast<common::SteadyTime::duration>(length),
                            QString::fromStdString(a->state));
    const bool pending = a->state == "Chờ duyệt";
    countdown_->setVisible(!pending);
    pendingSchedule_->setVisible(pending);
    pendingSchedule_->setText(pendingSchedule(*a));
    title_->setText(QString::fromStdString(a->productName));
    title_->setToolTip(title_->text());
    category_->setText(QString::fromStdString(a->category) + " · #" + QString::number(id_));
    const auto status = myStatus(*a, session_);
    price_->setAmount(a->currentPrice, status.contains("vượt"));
    counts_->setText(QString("%1 lượt đặt giá · %2 người tham gia")
                         .arg(a->bidHistory.size())
                         .arg(a->participants.size()));
    status_->setText(status);
    tone(status_, status.contains("vượt")      ? "danger"
                  : status.contains("dẫn đầu") ? "success"
                                               : "muted");
    open_->setText(isClosed(*a) ? "Xem kết quả" : "Xem phiên");
    setAccessibleName(title_->text() + " · " + price_->text());
    approve_->setVisible(manageApprove_ && (session_.loggedIn() && a->sellerId == session_.id()) && a->state == "Chờ duyệt");
    cancel_->setVisible(
        manageCancel_ && !isClosed(*a) &&
        (session_.loggedIn() && a->sellerId == session_.id()));
    approve_->setEnabled(store_.canWrite(id_));cancel_->setEnabled(store_.canWrite(id_));
    for(auto *b:{approve_,cancel_})b->setToolTip(store_.canWrite(id_)?QString():"Mất kết nối hoặc đang chờ máy chủ");
    if (hasJoined(*a, session_.id()))
        counts_->setText(
            counts_->text() + "\nCọc đang giữ: " +
            Formatters::formatMoney(isClosed(*a) ? domain::Money::zero() : a->depositAmount));
}
void AuctionCard::setManagement(bool approve, bool cancel) {
    manageApprove_ = approve;
    manageCancel_ = cancel;
    refresh();
}
void AuctionCard::setCompact(bool value) {
    if (value == compact_)
        return;
    compact_ = value;
    while (auto *item = content->takeAt(0))
        delete item;
    caption_->setVisible(!value);
    if (value) {
        image_->setFixedSize(96, 60);
        title_->setMinimumWidth(180);
        auto *row = new QHBoxLayout;
        row->setSpacing(16);
        row->addWidget(image_);
        auto *name = new QVBoxLayout;
        name->addWidget(title_);
        name->addWidget(category_);
        name->addWidget(status_);
        name->addWidget(pendingSchedule_);
        row->addLayout(name, 3);
        auto *money = new QVBoxLayout;
        money->addWidget(price_);
        money->addWidget(counts_);
        row->addLayout(money, 2);
        auto *state = new QVBoxLayout;
        state->addWidget(badge_);
        state->addWidget(countdown_);
        row->addLayout(state);
        auto *actions = new QVBoxLayout;
        actions->addWidget(open_);
        actions->addWidget(approve_);
        actions->addWidget(cancel_);
        row->addLayout(actions);
        content->addLayout(row);
    } else {
        image_->setMinimumSize(0, 100);
        image_->setMaximumSize(QWIDGETSIZE_MAX, 300);
        title_->setMinimumWidth(0);
        content->addWidget(image_);
        auto *row = new QHBoxLayout;
        row->addWidget(badge_);
        row->addStretch();
        row->addWidget(countdown_);
        content->addLayout(row);
        content->addWidget(pendingSchedule_);
        content->addWidget(title_);
        content->addWidget(category_);
        content->addWidget(caption_);
        content->addWidget(price_);
        content->addWidget(counts_);
        content->addWidget(status_);
        content->addWidget(open_);
        auto *actions = new QHBoxLayout;
        actions->addWidget(approve_);
        actions->addWidget(cancel_);
        content->addLayout(actions);
    }
    refresh();
    updateGeometry();
}
void AuctionCard::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton)
        emit openRequested(id_);
    Card::mouseReleaseEvent(e);
}
} // namespace auction::presentation
