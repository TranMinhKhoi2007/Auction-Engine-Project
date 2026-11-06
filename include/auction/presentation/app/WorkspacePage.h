#pragma once
#include "auction/presentation/app/NavigationController.h"
#include "auction/presentation/app/UiContext.h"
#include "auction/presentation/widgets/Components.h"

#include <QCompleter>
#include <QMenu>
#include <QStackedWidget>
#include <QStringListModel>
namespace auction::presentation {
class NavSidebar : public QFrame {
    Q_OBJECT
public:
    NavSidebar(UiContext &c, QWidget *parent = nullptr);
    void setCollapsed(bool collapsed, bool animate = true);
    bool collapsed() const { return collapsed_; }
    void refresh();
    void select(Page page);
signals:
    void navigate(Page page);
    void toggleRequested();

private:
    UiContext &c_;
    QMap<Page, QPushButton *> buttons_;
    QMap<Page, QString> names_, icons_;
    bool collapsed_{};
};
class TopBar : public QFrame {
    Q_OBJECT
public:
    TopBar(UiContext &c, QWidget *parent = nullptr);
    void refresh();
    void setTitle(const QString &title);
    QLineEdit *search;
    QLabel *notificationBadge;
    QMenu *profileMenu;
    QPushButton *bell;
    QPushButton *soundToggle;
signals:
    void createUserRequested();
    void topUpRequested();
    void logoutRequested();
    void inventoryRequested();
    void changePasswordRequested();
    void openRequested(quint64 id);

private:
    UiContext &c_;
    QLabel *title_;
    BalanceChip *balance_;
    QPushButton *profile_;
    QCompleter *completer_;
    QStringListModel *results_;
    QMap<QString, quint64> searchIds_;
};
class WorkspacePage : public QWidget {
public:
    WorkspacePage(UiContext &c, QWidget *parent = nullptr);
    NavSidebar *sidebar;
    TopBar *topbar;
    QStackedWidget *pages;
};
} // namespace auction::presentation
