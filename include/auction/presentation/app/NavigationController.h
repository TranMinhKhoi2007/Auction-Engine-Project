#pragma once
#include <QList>
#include <QObject>
namespace auction::presentation {
enum class Page {
    Home = -1,
    Marketplace = 0,
    Detail,
    MyAuctions,
    Seller,
    Inventory,
    Wallet,
    Notifications,
    Log
};
class NavigationController : public QObject {
    Q_OBJECT
public:
    explicit NavigationController(QObject *parent = nullptr) : QObject(parent) {}
    void go(Page page, quint64 auction = 0) {
        if (page == page_ && auction == auction_)
            return;
        history_.append({page_, auction_});
        page_ = page;
        auction_ = auction;
        emit changed(page_, auction_);
    }
    void back() {
        if (history_.isEmpty()) {
            go(Page::Marketplace);
            return;
        }
        auto last = history_.takeLast();
        page_ = last.first;
        auction_ = last.second;
        emit changed(page_, auction_);
    }
    Page page() const { return page_; }
    quint64 auctionId() const {return auction_;}
signals:
    void changed(Page page, quint64 auction);

private:
    Page page_{Page::Home};
    quint64 auction_{};
    QList<QPair<Page, quint64>> history_;
};
} // namespace auction::presentation
