#include "auction/presentation/pages/InventoryPage.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/theme/Fonts.h"
#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>

namespace auction::presentation {

// ============================================================================
// WonItemDetailDialog
// ============================================================================
WonItemDetailDialog::WonItemDetailDialog(const application::AuctionDetailDto &item, UiContext &context, QWidget *parent)
    : QDialog(parent), item_(item), c_(context) {
    setObjectName("wonItemDetailDialog");
    setWindowTitle("Kho lưu trữ · Chứng nhận sở hữu & Chi tiết vật phẩm");
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose);
    resize(840, 560);
    setMinimumSize(600, 380);

    const auto &theme = c_.theme.theme();
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(theme.bgSurface, theme.text));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(12);

    // --- Scroll Area for Dialog Content ---
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(QString(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 8px; background: %1; border-radius: 4px; margin: 0; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: %3; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    ).arg(theme.bgElevated, theme.border, theme.accent));

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setStyleSheet("background: transparent;");
    auto *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(6, 4, 12, 4);
    contentLayout->setSpacing(16);

    // --- Header Banner: Certificate style ---
    auto *certBanner = new QFrame(scrollContent);
    certBanner->setObjectName("certBanner");
    certBanner->setStyleSheet(QString(
        "QFrame#certBanner { background-color: %1; border: 1.5px solid %2; border-radius: 12px; padding: 14px; }"
    ).arg(theme.bgElevated, theme.accent));

    auto *certLayout = new QVBoxLayout(certBanner);
    certLayout->setContentsMargins(12, 8, 12, 8);
    certLayout->setSpacing(6);

    auto *certBadgeRow = new QHBoxLayout;
    auto *badgeCert = label("CHỨNG NHẬN TRÚNG ĐẤU GIÁ VÀ QUYỀN SỞ HỮU", certBanner, 11, "accent");
    badgeCert->setFont(Fonts::body(11, QFont::Bold));
    certBadgeRow->addWidget(badgeCert);
    certBadgeRow->addStretch();

    auto *catBadge = label(QString::fromStdString(item_.category), certBanner, 11);
    catBadge->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 3px 10px; font-weight: bold;"
    ).arg(theme.bgElevated, theme.accent, theme.border));
    certBadgeRow->addWidget(catBadge);
    certLayout->addLayout(certBadgeRow);

    auto *itemTitle = label(QString::fromStdString(item_.productName), certBanner, 22);
    itemTitle->setFont(Fonts::body(22, QFont::Bold));
    itemTitle->setWordWrap(true);
    certLayout->addWidget(itemTitle);

    auto *codeRow = label(QString("Mã vật phẩm: #P-%1   ·   Mã phiên đấu giá: #A-%2")
                              .arg(item_.productId.value())
                              .arg(item_.id.value()),
                          certBanner, 12, "muted");
    certLayout->addWidget(codeRow);
    contentLayout->addWidget(certBanner);

    // --- Content Body (Two Columns) ---
    auto *contentRow = new QHBoxLayout;
    contentRow->setSpacing(20);

    // Left Column: Image preview + Verification stamp
    auto *leftCol = new QVBoxLayout;
    leftCol->setSpacing(12);

    auto *imgContainer = new QFrame(scrollContent);
    imgContainer->setStyleSheet(QString(
        "border: 1px solid %1; border-radius: 10px; background-color: %2; overflow: hidden;"
    ).arg(theme.border, theme.bgElevated));
    auto *imgLayout = new QVBoxLayout(imgContainer);
    imgLayout->setContentsMargins(4, 4, 4, 4);

    auto *imageView = new ProductImageView(c_.images, imgContainer);
    imageView->setProduct(item_.id.value(), QString::fromStdString(item_.category));
    imageView->setMinimumSize(240, 180);
    imgLayout->addWidget(imageView);
    leftCol->addWidget(imgContainer);

    // Verification Seal Card
    auto *sealCard = new QFrame(scrollContent);
    sealCard->setStyleSheet(QString(
        "background-color: %1; border: 1px solid %2; border-radius: 8px; padding: 10px;"
    ).arg(theme.bgElevated, theme.success));
    auto *sealLayout = new QHBoxLayout(sealCard);
    sealLayout->setContentsMargins(8, 6, 8, 6);

    auto *sealIcon = new QLabel("✓", sealCard);
    sealIcon->setFont(Fonts::body(16, QFont::Bold));
    sealIcon->setStyleSheet(QString("color: %1;").arg(theme.success));
    sealLayout->addWidget(sealIcon);

    auto *sealText = label("XÁC NHẬN CHỦ SỞ HỮU HỢP PHÁP\nĐã hoàn tất thanh toán & chuyển giao quyền sở hữu.", sealCard, 11);
    sealText->setStyleSheet(QString("color: %1;").arg(theme.text));
    sealLayout->addWidget(sealText, 1);
    leftCol->addWidget(sealCard);
    leftCol->addStretch();

    contentRow->addLayout(leftCol, 4);

    // Right Column: Ownership Details & Product Description
    auto *rightCol = new QVBoxLayout;
    rightCol->setSpacing(14);

    // Transaction Details Box
    auto *txBox = new QFrame(scrollContent);
    txBox->setStyleSheet(QString(
        "background-color: %1; border: 1px solid %2; border-radius: 10px; padding: 14px;"
    ).arg(theme.bgElevated, theme.border));
    auto *txLayout = new QVBoxLayout(txBox);
    txLayout->setSpacing(8);

    auto *txTitle = label("THÔNG TIN SỞ HỮU & GIAO DỊCH", txBox, 12, "accent");
    txTitle->setFont(Fonts::body(12, QFont::Bold));
    txLayout->addWidget(txTitle);

    const QString ownerName = c_.session.user ? QString::fromStdString(c_.session.user->name) : "Bạn";
    const QString ownerEmail = c_.session.user ? QString::fromStdString(c_.session.user->email) : "";
    auto *ownerInfo = label(QString("Chủ sở hữu hiện tại:  %1 (%2)").arg(ownerName, ownerEmail), txBox, 13);
    ownerInfo->setFont(Fonts::body(13, QFont::DemiBold));
    txLayout->addWidget(ownerInfo);

    auto *priceRow = new QHBoxLayout;
    auto *lblP = label("Giá trúng đấu giá:", txBox, 13, "muted");
    auto *valP = label(Formatters::formatMoney(item_.currentPrice), txBox, 18, "accent");
    valP->setFont(Fonts::numbers(18, QFont::Bold));
    priceRow->addWidget(lblP);
    priceRow->addWidget(valP);
    priceRow->addStretch();
    txLayout->addLayout(priceRow);

    const QString wonTime = item_.endWallTime ? Formatters::formatWallTime(*item_.endWallTime) : "Đã hoàn tất";
    txLayout->addWidget(label(QString("Thời điểm trúng thầu:  %1").arg(wonTime), txBox, 12, "muted"));
    txLayout->addWidget(label(QString("Người bán chuyển giao:  %1").arg(QString::fromStdString(item_.sellerName)), txBox, 12, "muted"));

    const QString finishType = item_.closedEarly ? "Chốt sớm bởi người bán" : "Thắng phiên theo thời gian chính thức";
    txLayout->addWidget(label(QString("Hình thức hoàn tất:  %1").arg(finishType), txBox, 12, "muted"));

    rightCol->addWidget(txBox);

    // Description Box
    auto *descBox = new QFrame(scrollContent);
    descBox->setStyleSheet(QString(
        "background-color: %1; border: 1px solid %2; border-radius: 10px; padding: 14px;"
    ).arg(theme.bgElevated, theme.border));
    auto *descLayout = new QVBoxLayout(descBox);
    descLayout->setSpacing(6);

    auto *descTitle = label("MÔ TẢ CHI TIẾT VẬT PHẨM", descBox, 12, "accent");
    descTitle->setFont(Fonts::body(12, QFont::Bold));
    descLayout->addWidget(descTitle);

    auto *descText = new QLabel(QString::fromStdString(item_.productDescription), descBox);
    descText->setWordWrap(true);
    descText->setFont(Fonts::body(13));
    descText->setStyleSheet(QString("color: %1; line-height: 1.5;").arg(theme.text));
    descLayout->addWidget(descText);

    rightCol->addWidget(descBox);
    contentRow->addLayout(rightCol, 6);

    contentLayout->addLayout(contentRow);
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // --- Footer Action Buttons ---
    auto *footer = new QHBoxLayout;
    footer->setSpacing(12);

    auto *btnViewAuction = button("Xem lại phiên đấu giá gốc", "btnDialogViewAuction", this, "ghost");
    footer->addWidget(btnViewAuction);
    connect(btnViewAuction, &QPushButton::clicked, this, [this] {
        emit openAuctionRequested(item_.id.value());
        accept();
    });

    auto *btnResell = button("🏷️ Bán lại vật phẩm", "btnDialogResell", this, "primary");
    btnResell->setToolTip("Đăng bán lại vật phẩm này lên sàn đấu giá với thông tin kế thừa từ phiên thắng");
    footer->addWidget(btnResell);
    connect(btnResell, &QPushButton::clicked, this, [this] {
        emit resellRequested(item_);
        accept();
    });

    footer->addStretch();

    auto *btnClose = button("Đóng", "btnDialogClose", this, "primary");
    btnClose->setFixedWidth(120);
    footer->addWidget(btnClose);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    mainLayout->addLayout(footer);
}

// ============================================================================
// WonItemCard
// ============================================================================
WonItemCard::WonItemCard(const application::AuctionDetailDto &item, UiContext &context, QWidget *parent)
    : Card(parent, true), id_(item.id.value()) {
    setObjectName(QString("wonItemCard_%1").arg(id_));
    setCursor(Qt::PointingHandCursor);
    setFixedWidth(290);

    const auto &theme = context.theme.theme();
    content->setContentsMargins(12, 12, 12, 12);
    content->setSpacing(8);

    // Image preview
    image_ = new ProductImageView(context.images, this);
    image_->setProduct(id_, QString::fromStdString(item.category));
    image_->setFixedHeight(160);
    content->addWidget(image_);

    // Badges Row: Category + Ownership tag
    auto *badgeRow = new QHBoxLayout;
    categoryBadge_ = label(QString::fromStdString(item.category), this, 11, "muted");
    categoryBadge_->setStyleSheet(QString(
        "background-color: %1; border-radius: 4px; padding: 2px 8px; font-weight: bold;"
    ).arg(theme.bgElevated));
    badgeRow->addWidget(categoryBadge_);
    badgeRow->addStretch();

    ownershipBadge_ = label("✓ ĐÃ SỞ HỮU", this, 11);
    ownershipBadge_->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid %2; border-radius: 4px; padding: 2px 8px; font-weight: bold;"
    ).arg(theme.bgElevated, theme.success));
    badgeRow->addWidget(ownershipBadge_);
    content->addLayout(badgeRow);

    // Title
    title_ = label(QString::fromStdString(item.productName), this, 15);
    title_->setFont(Fonts::body(15, QFont::Bold));
    title_->setWordWrap(false);
    title_->setToolTip(QString::fromStdString(item.productName));
    content->addWidget(title_);

    // Winning Price
    auto *priceRow = new QHBoxLayout;
    priceRow->addWidget(label("Giá trúng:", this, 12, "muted"));
    priceLabel_ = label(Formatters::formatMoney(item.currentPrice), this, 16, "accent");
    priceLabel_->setFont(Fonts::numbers(16, QFont::Bold));
    priceRow->addWidget(priceLabel_);
    priceRow->addStretch();
    content->addLayout(priceRow);

    // Date & Seller info
    const QString wonTime = item.endWallTime ? Formatters::formatWallTime(*item.endWallTime) : "Hoàn tất";
    timeLabel_ = label("Ngày thắng: " + wonTime.section(' ', 0, 0), this, 11, "muted");
    content->addWidget(timeLabel_);

    sellerLabel_ = label("Người bán: " + QString::fromStdString(item.sellerName), this, 11, "muted");
    sellerLabel_->setToolTip(sellerLabel_->text());
    content->addWidget(sellerLabel_);

    // Actions
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    btnOpen_ = button("Mở ra xem", QString("btnOpenWon_%1").arg(id_), this, "primary");
    btnOpen_->setToolTip("Mở chứng nhận sở hữu và thông tin chi tiết vật phẩm");
    btnRow->addWidget(btnOpen_, 6);

    btnAuction_ = button("Phiên", QString("btnAuctionWon_%1").arg(id_), this, "ghost");
    btnAuction_->setToolTip("Xem lại phiên đấu giá gốc");
    btnRow->addWidget(btnAuction_, 4);

    content->addLayout(btnRow);

    connect(btnOpen_, &QPushButton::clicked, this, [this] {
        emit viewDetailRequested(id_);
    });
    connect(btnAuction_, &QPushButton::clicked, this, [this] {
        emit openAuctionRequested(id_);
    });
}

void WonItemCard::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit viewDetailRequested(id_);
        event->accept();
        return;
    }
    Card::mouseReleaseEvent(event);
}

// ============================================================================
// InventoryPage
// ============================================================================
InventoryPage::InventoryPage(UiContext &context, QWidget *parent)
    : QWidget(parent), c_(context) {
    setObjectName("inventoryPage");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(18);

    // --- Heading Row ---
    auto *heading = new QHBoxLayout;
    auto *headingText = new QVBoxLayout;
    headingText->setSpacing(4);

    auto *titleRow = new QHBoxLayout;
    auto *mainTitle = label("Kho lưu trữ vật phẩm", this, 28);
    mainTitle->setFont(Fonts::body(28, QFont::Bold));
    titleRow->addWidget(mainTitle);

    countBadge_ = label("0 món", this, 12);
    countBadge_->setObjectName("lblInventoryCount");
    countBadge_->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid %2; border-radius: 10px; padding: 3px 12px; font-weight: bold;"
    ).arg(c_.theme.theme().bgElevated, c_.theme.theme().accent));
    titleRow->addWidget(countBadge_);
    titleRow->addStretch();
    headingText->addLayout(titleRow);

    auto *subTitle = label("Bộ sưu tập những món hàng bạn đã đấu giá thành công và chính thức sở hữu.",
                           this, 14, "muted");
    headingText->addWidget(subTitle);
    heading->addLayout(headingText, 1);
    mainLayout->addLayout(heading);

    // --- Summary Stat Cards Row ---
    auto *statRow = new QHBoxLayout;
    statRow->setSpacing(14);

    auto *statCount = new StatCard("Tổng vật phẩm đã sở hữu", "award", c_.theme, this);
    stats_.append(statCount);
    statRow->addWidget(statCount, 1);

    auto *statTotal = new StatCard("Tổng giá trị đã trúng thầu", "wallet", c_.theme, this);
    stats_.append(statTotal);
    statRow->addWidget(statTotal, 1);

    auto *statTop = new StatCard("Vật phẩm giá trị nhất", "trending-up", c_.theme, this);
    stats_.append(statTop);
    statRow->addWidget(statTop, 1);

    mainLayout->addLayout(statRow);

    // --- Search & Filters Bar ---
    auto *filterBar = new QHBoxLayout;
    filterBar->setSpacing(12);

    search_ = new QLineEdit(this);
    search_->setObjectName("edtInventorySearch");
    search_->setPlaceholderText("Tìm kiếm vật phẩm trong kho theo tên, người bán…");
    search_->setClearButtonEnabled(true);
    search_->setMinimumWidth(280);
    filterBar->addWidget(search_, 1);

    categoryFilter_ = new QComboBox(this);
    categoryFilter_->setObjectName("cmbInventoryCategory");
    categoryFilter_->addItems({"Tất cả danh mục", "Đồ cổ", "Tranh nghệ thuật", "Thiết bị công nghệ"});
    filterBar->addWidget(categoryFilter_);

    sortOrder_ = new QComboBox(this);
    sortOrder_->setObjectName("cmbInventorySort");
    sortOrder_->addItems({"Mới nhất", "Cũ nhất", "Giá cao → thấp", "Giá thấp → cao", "Tên A-Z"});
    filterBar->addWidget(sortOrder_);

    mainLayout->addLayout(filterBar);

    connect(search_, &QLineEdit::textChanged, this, [this] { refresh(); });
    connect(categoryFilter_, &QComboBox::currentIndexChanged, this, [this] { refresh(); });
    connect(sortOrder_, &QComboBox::currentIndexChanged, this, [this] { refresh(); });

    // --- Scrollable Grid Content ---
    auto *body = new QWidget(this);
    body->setObjectName("inventoryBody");
    auto *content = new QVBoxLayout(body);
    content->setContentsMargins(0, 0, 0, 0);

    grid_ = new QWidget(body);
    grid_->setObjectName("inventoryGrid");
    flow_ = new FlowLayout(grid_);
    flow_->setListMode(false);
    content->addWidget(grid_);

    empty_ = new EmptyState(
        "Kho lưu trữ của bạn đang trống",
        "Bạn chưa trúng đấu giá món hàng nào. Hãy tham gia đặt giá trên sàn để sở hữu những vật phẩm giá trị!",
        "Khám phá sàn đấu giá",
        c_.theme, body, "award");
    content->addWidget(empty_);
    content->addStretch();

    mainLayout->addWidget(scrollPage(body, this), 1);

    connect(empty_->action, &QPushButton::clicked, this, &InventoryPage::exploreMarketplaceRequested);

    // Connect reactive updates from data store and session
    connect(&c_.store, &AuctionStore::auctionChanged, this, [this](quint64) { refresh(); });
    connect(&c_.store, &AuctionStore::auctionListChanged, this, &InventoryPage::refresh);
    connect(&c_.store, &AuctionStore::personalChanged, this, &InventoryPage::refresh);
    connect(&c_.session, &UserSession::changed, this, &InventoryPage::refresh);

    refresh();
}

void InventoryPage::openWonItemDetail(quint64 auctionId) {
    const auto *auction = c_.store.get(auctionId);
    if (!auction)
        return;

    auto *dialog = new WonItemDetailDialog(*auction, c_, this);
    connect(dialog, &WonItemDetailDialog::openAuctionRequested, this, &InventoryPage::openAuctionRequested);
    connect(dialog, &WonItemDetailDialog::resellRequested, this, &InventoryPage::resellRequested);
    dialog->open();
}

void InventoryPage::refresh() {
    if (!c_.session.loggedIn()) {
        empty_->show();
        grid_->hide();
        countBadge_->setText("0 món");
        if (stats_.size() >= 3) {
            stats_[0]->value->setText("0 món");
            stats_[1]->value->setText("0 VND");
            stats_[2]->value->setText("Chưa có");
        }
        for (auto *card : cards_)
            card->hide();
        return;
    }

    const auto userId = c_.session.id();
    std::vector<application::AuctionDetailDto> wonItems;

    for (const auto &a : c_.store.auctions()) {
        if (a.outcome == domain::AuctionOutcome::Sold && a.leaderId.has_value() && *a.leaderId == userId) {
            wonItems.push_back(a);
        }
    }

    // Update Statistics
    const int totalCount = static_cast<int>(wonItems.size());
    countBadge_->setText(QString("%1 món").arg(totalCount));

    domain::Money totalSpent = domain::Money::zero();
    domain::Money maxSpent = domain::Money::zero();
    QString maxItemTitle = "Chưa có";

    for (const auto &item : wonItems) {
        totalSpent = totalSpent + item.currentPrice;
        if (item.currentPrice > maxSpent) {
            maxSpent = item.currentPrice;
            maxItemTitle = QString::fromStdString(item.productName);
        }
    }

    if (stats_.size() >= 3) {
        stats_[0]->value->setText(QString("%1 món").arg(totalCount));
        stats_[1]->value->setText(Formatters::formatMoney(totalSpent));
        stats_[2]->value->setText(totalCount > 0 ? Formatters::formatMoney(maxSpent) : "Chưa có");
        stats_[2]->value->setToolTip(totalCount > 0 ? maxItemTitle : "");
    }

    // Apply Filters & Search
    const QString search = search_->text().trimmed();
    const QString category = categoryFilter_->currentIndex() > 0 ? categoryFilter_->currentText() : QString();

    std::vector<application::AuctionDetailDto> visible;
    for (const auto &item : wonItems) {
        if (!category.isEmpty() && QString::fromStdString(item.category) != category)
            continue;
        if (!search.isEmpty()) {
            const QString title = QString::fromStdString(item.productName);
            const QString desc = QString::fromStdString(item.productDescription);
            const QString seller = QString::fromStdString(item.sellerName);
            if (!title.contains(search, Qt::CaseInsensitive) &&
                !desc.contains(search, Qt::CaseInsensitive) &&
                !seller.contains(search, Qt::CaseInsensitive))
                continue;
        }
        visible.push_back(item);
    }

    // Apply Sorting
    const int sortIdx = sortOrder_->currentIndex();
    std::sort(visible.begin(), visible.end(), [sortIdx](const auto &a, const auto &b) {
        if (sortIdx == 0) { // Mới nhất
            return a.id.value() > b.id.value();
        } else if (sortIdx == 1) { // Cũ nhất
            return a.id.value() < b.id.value();
        } else if (sortIdx == 2) { // Giá cao -> thấp
            return a.currentPrice > b.currentPrice;
        } else if (sortIdx == 3) { // Giá thấp -> cao
            return a.currentPrice < b.currentPrice;
        } else { // Tên A-Z
            return a.productName < b.productName;
        }
    });

    // Update Grid Cards
    QSet<quint64> activeIds;
    for (const auto &item : visible) {
        const auto id = item.id.value();
        activeIds.insert(id);

        if (!cards_.contains(id)) {
            auto *card = new WonItemCard(item, c_, grid_);
            connect(card, &WonItemCard::viewDetailRequested, this, &InventoryPage::openWonItemDetail);
            connect(card, &WonItemCard::openAuctionRequested, this, &InventoryPage::openAuctionRequested);
            cards_.insert(id, card);
            flow_->addWidget(card);
        }
        cards_[id]->show();
    }

    for (auto it = cards_.begin(); it != cards_.end(); ++it) {
        if (!activeIds.contains(it.key())) {
            it.value()->hide();
        }
    }

    const bool hasVisible = !visible.empty();
    grid_->setVisible(hasVisible);
    empty_->setVisible(!hasVisible);
}

} // namespace auction::presentation
