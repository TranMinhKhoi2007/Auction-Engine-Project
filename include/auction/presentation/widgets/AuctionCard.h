#pragma once
#include "auction/presentation/state/AuctionStore.h"
#include "auction/presentation/state/ProductImageStore.h"
#include "auction/presentation/widgets/Components.h"
namespace auction::presentation {
class ProductImageView : public QWidget {
    Q_OBJECT
public:
    ProductImageView(ProductImageStore &images, QWidget *parent = nullptr);
    void setProduct(quint64 id, const QString &category);
    QSize sizeHint() const override { return {280, 200}; }
    int heightForWidth(int w) const override {
        const int maxH = (maximumHeight() > 0 && maximumHeight() < 16777215) ? maximumHeight() : 480;
        const int minH = minimumHeight() > 0 ? minimumHeight() : 100;
        return qBound(minH, w * 10 / 16, maxH);
    }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    ProductImageStore &images_;
    quint64 id_{};
    QString category_;
    QPixmap cached_;
    QSize cachedSize_;
    qreal cachedDpr_{};
};
class AuctionCard : public Card {
    Q_OBJECT
public:
    AuctionCard(quint64 id, AuctionStore &store, UserSession &session, UiTicker &ticker,
                ThemeManager &theme, ProductImageStore &images, QWidget *parent = nullptr);
    void refresh();
    void setCompact(bool value);
    void setManagement(bool approve, bool cancel);
    quint64 id() const { return id_; }
signals:
    void openRequested(quint64 id);
    void approveRequested(quint64 id);
    void cancelRequested(quint64 id);

protected:
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    quint64 id_;
    AuctionStore &store_;
    UserSession &session_;
    ProductImageView *image_;
    StateBadge *badge_;
    CountdownLabel *countdown_;
    QLabel *pendingSchedule_;
    PriceLabel *price_;
    QLabel *title_, *category_, *caption_, *counts_, *status_;
    QPushButton *open_, *approve_, *cancel_;
    bool manageApprove_{false}, manageCancel_{false}, compact_{false};
};
} // namespace auction::presentation
