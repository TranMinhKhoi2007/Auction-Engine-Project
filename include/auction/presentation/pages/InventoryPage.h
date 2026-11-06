#pragma once

#include "auction/application/Dtos.h"
#include "auction/presentation/app/UiContext.h"
#include "auction/presentation/widgets/AuctionCard.h"
#include "auction/presentation/widgets/Components.h"
#include "auction/presentation/widgets/FlowLayout.h"

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QMap>
#include <QScrollArea>
#include <vector>

namespace auction::presentation {

// Dialog "Mở ra xem" - Hiển thị chứng nhận sở hữu và chi tiết đầy đủ của vật phẩm đã đấu giá thành công
class WonItemDetailDialog : public QDialog {
    Q_OBJECT
public:
    WonItemDetailDialog(const application::AuctionDetailDto &item, UiContext &context, QWidget *parent = nullptr);

signals:
    void openAuctionRequested(quint64 auctionId);
    void resellRequested(const application::AuctionDetailDto &item);

private:
    const application::AuctionDetailDto item_;
    UiContext &c_;
};

// Card đại diện cho một món hàng trong kho lưu trữ
class WonItemCard : public Card {
    Q_OBJECT
public:
    WonItemCard(const application::AuctionDetailDto &item, UiContext &context, QWidget *parent = nullptr);
    quint64 auctionId() const { return id_; }

signals:
    void viewDetailRequested(quint64 auctionId);
    void openAuctionRequested(quint64 auctionId);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    quint64 id_;
    ProductImageView *image_;
    QLabel *title_;
    QLabel *categoryBadge_;
    QLabel *ownershipBadge_;
    QLabel *priceLabel_;
    QLabel *timeLabel_;
    QLabel *sellerLabel_;
    QPushButton *btnOpen_;
    QPushButton *btnAuction_;
};

// Trang Kho lưu trữ vật phẩm đã đấu giá được
class InventoryPage : public QWidget {
    Q_OBJECT
public:
    explicit InventoryPage(UiContext &context, QWidget *parent = nullptr);
    void refresh();

signals:
    void openAuctionRequested(quint64 id);
    void exploreMarketplaceRequested();
    void resellRequested(const application::AuctionDetailDto &item);

private:
    void openWonItemDetail(quint64 auctionId);

    UiContext &c_;
    QLineEdit *search_;
    QComboBox *categoryFilter_;
    QComboBox *sortOrder_;
    FlowLayout *flow_;
    QWidget *grid_;
    EmptyState *empty_;
    QList<StatCard *> stats_;
    QMap<quint64, WonItemCard *> cards_;
    QList<quint64> order_;
    QLabel *countBadge_;
};

} // namespace auction::presentation
