#pragma once
#include "auction/presentation/app/UiContext.h"
#include "auction/presentation/widgets/AuctionCard.h"

#include <QBoxLayout>
#include <QListView>
#include <QProgressBar>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTabWidget>
namespace auction::presentation {
class BidHistoryModel;
class StateLifecycleWidget;
class AuctionDetailPage : public QScrollArea {
    Q_OBJECT
public:
    AuctionDetailPage(UiContext &context, QWidget *parent = nullptr);
    void openAuction(quint64 id);
    void refresh();
    void setError(const QString &error, const QString &action = {});
    quint64 auctionId() const { return id_; }
    MoneyLineEdit *bidInput() const { return amount_; }
    void submitBid();
signals:
    void backRequested();
    void joinRequested(quint64 id);
    void bidRequested(quint64 id, qint64 amount);
    void approveRequested(quint64 id);
    void cancelRequested(quint64 id);
    void closeEarlyRequested(quint64 id);
    void followRequested(quint64 id);
    void topUpRequested();
    void deleteRequested(quint64 id);

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    QLabel *loading_{};
    void refreshTime();
    void updateColumns();
    UiContext &c_;
    quint64 id_{};
    bool stacked_{false};
    bool amountDirty_{false};
    QWidget *body_, *left_, *right_, *bidControls_, *ownerControls_;
    QBoxLayout *columns_;
    ProductImageView *image_;
    StateBadge *badge_;
    CountdownLabel *countdown_;
    PriceLabel *price_;
    StepIndicator *steps_;
    QProgressBar *progress_;
    QLabel *title_, *category_, *description_, *leader_, *myStatus_, *timeCaption_, *result_,
        *error_, *historyEmpty_, *closeReason_, *earlyBadge_;
    StateLifecycleWidget *lifecycleWidget_{nullptr};
    QLabel *auctioneerCallout_{nullptr};
    QLabel *gavelBanner_{nullptr};
    int lastCountdownSec_{-1};
    quint64 lastGavelAuctionId_{0};
    QListView *participants_;
    QStandardItemModel *participantsModel_;
    QList<QLabel *> params_;
    MoneyLineEdit *amount_;
    QPushButton *join_, *bid_, *approve_, *cancel_, *follow_, *fix_, *closeEarly_, *deleteAuctionButton_;
    QListView *history_;
    BidHistoryModel *historyModel_;
};
} // namespace auction::presentation
