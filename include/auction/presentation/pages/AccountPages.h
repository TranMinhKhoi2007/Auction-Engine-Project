#pragma once
#include "auction/presentation/app/UiContext.h"
#include "auction/presentation/widgets/Components.h"

#include <QCheckBox>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
namespace auction::presentation {
class WalletPage : public QWidget {
    Q_OBJECT
public:
    WalletPage(UiContext &c, QWidget *parent = nullptr);
    void refresh();
signals:
    void topUpRequested();
    void openRequested(quint64 id);

private:
    UiContext &c_;
    QList<StatCard *> stats_;
    QTableView *table_;
    QStandardItemModel *model_;
    EmptyState *empty_;
    QPushButton *topUp_;
};
class NotificationsPage : public QWidget {
    Q_OBJECT
public:
    NotificationsPage(UiContext &c, QWidget *parent = nullptr);
    void refresh();
    void markAllRead();
    int unread() const;
    QStringList latest() const;
    void openLatest(int index);
signals:
    void openRequested(quint64 id);
    void unreadChanged(int count);

private:
    UiContext &c_;
    QListView *list_;
    QStandardItemModel *model_;
    EmptyState *empty_;
    QMap<quint64, QSet<QString>> read_;
};
class LogModel;
class LogFilter;
class SystemLogPage : public QWidget {
    Q_OBJECT
public:
    SystemLogPage(UiContext &c, QWidget *parent = nullptr);
    void appendLog(const QString &level, const QString &message);

private:
    UiContext &c_;
    QListView *list_;
    LogModel *model_;
    LogFilter *proxy_;
    QCheckBox *autoscroll_;
    EmptyState *empty_;
};
} // namespace auction::presentation
