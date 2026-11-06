#pragma once
#include "auction/domain/values/Money.h"
#include "auction/presentation/state/UiTicker.h"
#include "auction/presentation/theme/ThemeManager.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <functional>
#include <optional>
namespace auction::presentation {
class Card : public QFrame {
public:
    explicit Card(QWidget *parent = nullptr, bool elevated = false);
    QVBoxLayout *content;
};
class ElidedLabel : public QLabel {
public:
    ElidedLabel(QWidget *parent, int pixels);
protected:
    void paintEvent(QPaintEvent *) override;
};
class Chip : public QPushButton {
public:
    Chip(const QString &text, QWidget *parent = nullptr);
};
class StateBadge : public QLabel {
public:
    StateBadge(ThemeManager &theme, QWidget *parent = nullptr);
    void setState(const QString &state);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString state_;
    ThemeManager &theme_;
};
class AvatarCircle : public QWidget {
public:
    AvatarCircle(quint64 id, const QString &name, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    quint64 id_;
    QString initials_;
};
class CountdownLabel : public QLabel {
public:
    CountdownLabel(UiTicker &ticker, QWidget *parent = nullptr, bool hero = false);
    void setDeadline(common::SteadyTime end, common::SteadyTime start, const QString &state);
    void refresh(common::SteadyTime now);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    UiTicker &ticker_;
    common::SteadyTime end_{}, start_{};
    QString state_;
    qreal opacity_{1};
};
class BalanceChip : public QLabel {
public:
    explicit BalanceChip(QWidget *parent = nullptr);
    void setBalance(const QString &text, const QString &details);
protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    QString fullText_;
};
class ClockChip : public QLabel {
public:
    ClockChip(UiTicker &ticker, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    qreal pulse_{1};
};
class PriceLabel : public QLabel {
public:
    PriceLabel(ThemeManager &theme, QWidget *parent = nullptr, int pixels = 36);
    void setAmount(domain::Money value, bool outbid = false);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *) override;

private:
    void fitText();
    ThemeManager &theme_;
    std::optional<int64_t> value_;
    QVariantAnimation animation_;
    QColor flash_{Qt::transparent};
    int pixels_;
};
class MoneyLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit MoneyLineEdit(QWidget *parent = nullptr);
    qint64 value() const;
    void setValue(qint64 value);
signals:
    void valueChanged(qint64 value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void formatInput(const QString &text);
    QString previous_{"0"};
};
class StepIndicator : public QWidget {
public:
    StepIndicator(const QStringList &steps, QWidget *parent = nullptr);
    void setStep(int step);

private:
    QList<QLabel *> labels_;
};
class StatCard : public Card {
public:
    StatCard(const QString &title, const QString &icon, ThemeManager &theme,
             QWidget *parent = nullptr);
    QLabel *value;
};
class EmptyState : public Card {
public:
    EmptyState(const QString &title, const QString &description, const QString &action,
               ThemeManager &theme, QWidget *parent = nullptr, const QString &icon = "gavel");
    QPushButton *action;
};
class ToastManager : public QWidget {
    Q_OBJECT
public:
    ToastManager(UiTicker &ticker, ThemeManager &theme, QWidget *parent);
    void showToast(const QString &message, const QString &tone = "info", const QString &action = {},
                   std::function<void()> callback = {});
    void reposition();

private:
    struct Toast {
        QPointer<Card> card;
        common::SteadyTime expiry;
    };
    QList<Toast> toasts_;
    QVBoxLayout *layout_;
    UiTicker &ticker_;
    ThemeManager &theme_;
};
} // namespace auction::presentation
