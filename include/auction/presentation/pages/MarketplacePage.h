#pragma once
#include "auction/presentation/app/UiContext.h"
#include "auction/presentation/widgets/AuctionCard.h"
#include "auction/presentation/widgets/FlowLayout.h"

#include <QComboBox>
#include <QTabBar>
namespace auction::presentation {
class MarketplacePage : public QWidget {
    Q_OBJECT
public:
    enum class Mode { Market, Mine, Seller };
    MarketplacePage(UiContext &context, QWidget *parent = nullptr, Mode mode = Mode::Market);
    void setSearch(const QString &search);
    void refresh();
signals:
    void openRequested(quint64 id);
    void createRequested();
    void createUserRequested();
    void approveRequested(quint64 id);
    void cancelRequested(quint64 id);
    void seedDemoRequested();

private:
    UiContext &c_;
    Mode mode_;
    QString search_, state_;
    QComboBox *category_, *sort_;
    QPushButton *create_{nullptr}, *seedDemo_{nullptr};
    QList<Chip *> chips_;
    QTabBar *tabs_;
    FlowLayout *flow_;
    QMap<quint64, AuctionCard *> cards_;
    QList<quint64> order_;
    EmptyState *empty_;
    QWidget *grid_;
    QList<StatCard *> stats_;
    bool list_{false};
};
class MyAuctionsPage : public MarketplacePage {
public:
    MyAuctionsPage(UiContext &c, QWidget *p = nullptr) : MarketplacePage(c, p, Mode::Mine) {
        setObjectName("myAuctionsPage");
    }
};
class SellerDashboardPage : public MarketplacePage {
public:
    SellerDashboardPage(UiContext &c, QWidget *p = nullptr) : MarketplacePage(c, p, Mode::Seller) {
        setObjectName("sellerDashboardPage");
    }
};
} // namespace auction::presentation
