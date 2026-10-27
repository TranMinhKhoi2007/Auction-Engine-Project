#pragma once
#include "auction/presentation/app/UiContext.h"
#include "auction/presentation/widgets/AuctionCard.h"
#include "auction/presentation/widgets/FlowLayout.h"

#include <QScrollArea>
namespace auction::presentation {
class HomePage : public QScrollArea {
    Q_OBJECT
public:
    HomePage(UiContext &context, QWidget *parent = nullptr);
    void refresh();
signals:
    void enterRequested();
    void openRequested(quint64 id);
    void quickStartRequested(int step);

private:
    UiContext &c_;
    QWidget *featureHost_;
    QVBoxLayout *featureLayout_;
    EmptyState *featureEmpty_;
    AuctionCard *featured_{nullptr};
    quint64 featuredId_{};
    QList<StatCard *> stats_;
    QWidget *closing_;
    FlowLayout *closingLayout_;
    QMap<quint64, AuctionCard *> cards_;
    Card *gettingStarted_;
    QList<QPushButton *> steps_;
    QLabel *featureTitleLabel_{nullptr};
    QLabel *closingTitleLabel_{nullptr};
};
} // namespace auction::presentation
