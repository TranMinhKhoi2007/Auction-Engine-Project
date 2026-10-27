#include "auction/presentation/widgets/Components.h"

#include "auction/presentation/Formatters.h"
#include "auction/presentation/theme/Fonts.h"
#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPropertyAnimation>
#include <QRegularExpressionValidator>
#include <cmath>
#include <limits>
namespace auction::presentation {
ElidedLabel::ElidedLabel(QWidget *parent, int pixels) : QLabel(parent) {
    setFont(Fonts::body(pixels, pixels >= 16 ? QFont::DemiBold : QFont::Normal));
    setTextFormat(Qt::PlainText);
    setWordWrap(false);
    setMinimumHeight(fontMetrics().height() + 2);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
}
void ElidedLabel::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setFont(font());
    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(contentsRect(), Qt::AlignLeft | Qt::AlignVCenter,
                     fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width()));
}
BalanceChip::BalanceChip(QWidget *parent) : QLabel(parent) {
    setFont(Fonts::body(13));
    setProperty("tone", "accent");
    setFixedWidth(210);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}
void BalanceChip::setBalance(const QString &text, const QString &details) {
    fullText_ = text;
    setAccessibleName(text);
    setToolTip(text + "\n" + details);
    setText(fontMetrics().elidedText(fullText_, Qt::ElideRight, width()));
}
void BalanceChip::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    setText(fontMetrics().elidedText(fullText_, Qt::ElideRight, width()));
}

Card::Card(QWidget *parent, bool elevated) : QFrame(parent) {
    setProperty("card", true);
    setProperty("elevated", elevated);
    content = new QVBoxLayout(this);
    content->setContentsMargins(20, 20, 20, 20);
    content->setSpacing(12);
}
Chip::Chip(const QString &text, QWidget *parent) : QPushButton(text, parent) {
    setCheckable(true);
    setProperty("chip", true);
    setAccessibleName(text);
    setCursor(Qt::PointingHandCursor);
}
StateBadge::StateBadge(ThemeManager &theme, QWidget *parent) : QLabel(parent), theme_(theme) {
    setObjectName("badgeState");
    setProperty("badge", true);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    connect(&theme_, &ThemeManager::changed, this, [this] { setState(state_); });
}
void StateBadge::setState(const QString &state) {
    state_ = state;
    QString text = state;
    QString color = "muted";
    QString symbol = "check";
    if (state == "Đang diễn ra") {
        text = "LIVE · Đang diễn ra";
        color = "success";
        symbol = "activity";
    } else if (state == "Chờ duyệt") {
        color = "warning";
        symbol = "clock";
    } else if (state == "Sắp diễn ra") {
        color = "info";
        symbol = "clock";
    } else if (state == "Hủy") {
        text = "Đã hủy";
        color = "danger";
        symbol = "x";
    } else if (state == "Kết thúc")
        text = "Đã kết thúc";
    setText(text);
    setAccessibleName(text);
    setProperty("stateIcon", symbol);
    tone(this, color);
}
void StateBadge::paintEvent(QPaintEvent *event) {
    QLabel::paintEvent(event);
    QPainter p(this);
    IconProvider::icon(property("stateIcon").toString(), palette().color(QPalette::WindowText), 14)
        .paint(&p, 8, (height() - 14) / 2, 14, 14);
}
AvatarCircle::AvatarCircle(quint64 id, const QString &name, QWidget *parent)
    : QWidget(parent), id_(id) {
    auto words = name.simplified().split(' ', Qt::SkipEmptyParts);
    if (!words.isEmpty()) {
        initials_ = words.first().left(1);
        if (words.size() > 1)
            initials_ += words.last().left(1);
    }
    setFixedSize(36, 36);
    setToolTip(name);
    setAccessibleName(name);
}
void AvatarCircle::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    auto colors = Theme::avatarColors();
    p.setBrush(QColor(colors.at(static_cast<int>(id_ % 8))));
    p.setPen(Qt::NoPen);
    p.drawEllipse(rect());
    p.setPen(Qt::white);
    p.setFont(Fonts::body(12, QFont::DemiBold));
    p.drawText(rect(), Qt::AlignCenter, initials_);
}
CountdownLabel::CountdownLabel(UiTicker &ticker, QWidget *parent, bool hero)
    : QLabel(parent), ticker_(ticker) {
    setFont(Fonts::numbers(hero ? 32 : 14, QFont::DemiBold));
    setObjectName("lblCountdown");
    connect(&ticker, &UiTicker::tick, this,
            [this](common::WallTime, common::SteadyTime s) { refresh(s); });
}
void CountdownLabel::setDeadline(common::SteadyTime end, common::SteadyTime start,
                                 const QString &state) {
    end_ = end;
    start_ = start;
    state_ = state;
    refresh(ticker_.clock().now());
}
void CountdownLabel::refresh(common::SteadyTime now) {
    QString result;
    QString color;
    if (state_ == "Chờ duyệt")
        result = "Chưa có lịch";
    else if (state_ == "Hủy")
        result = "Đã hủy";
    else if (state_ == "Kết thúc")
        result = "Đã kết thúc";
    else {
        auto seconds = std::max<int64_t>(
            0, (std::chrono::duration_cast<common::Millis>((now < start_ ? start_ : end_) - now)
                    .count() +
                999) /
                   1000);
        result = QString("%1:%2:%3")
                     .arg(seconds / 3600, 2, 10, QChar('0'))
                     .arg(seconds / 60 % 60, 2, 10, QChar('0'))
                     .arg(seconds % 60, 2, 10, QChar('0'));
        color = now < start_ ? "info" : seconds <= 10 ? "danger" : seconds <= 60 ? "warning" : "";
        setToolTip(now < start_ ? "Bắt đầu sau" : "Kết thúc sau");
    }
    if (text() != result)
        setText(result);
    tone(this, color);
    const auto phase =
        std::chrono::duration_cast<common::Millis>(now.time_since_epoch()).count() % 1000;
    const qreal opacity =
        color == "danger" ? 0.9 + 0.1 * std::abs(std::sin(phase * 3.14159265 / 1000)) : 1;
    if (opacity != opacity_) {
        opacity_ = opacity;
        update();
    }
}
void CountdownLabel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setOpacity(opacity_);
    p.setFont(font());
    p.setPen(palette().color(QPalette::WindowText));
    p.drawText(contentsRect(), Qt::AlignLeft | Qt::AlignVCenter, text());
}
ClockChip::ClockChip(UiTicker &ticker, QWidget *parent) : QLabel(parent) {
    setFont(Fonts::numbers(12));
    tone(this, "muted");
    setContentsMargins(14, 0, 0, 0);
    auto refresh = [this](common::WallTime wall, common::SteadyTime steady) {
        const auto text = "LIVE  ·  " + Formatters::formatWallTime(wall);
        if (this->text() != text)
            setText(text);
        pulse_ =
            0.5 +
            0.5 *
                std::abs(std::sin(
                    (std::chrono::duration_cast<common::Millis>(steady.time_since_epoch()).count() %
                     1000) *
                    3.14159265 / 1000));
        update();
    };
    connect(&ticker, &UiTicker::tick, this, refresh);
    refresh(ticker.clock().wallNow(), ticker.clock().now());
}
void ClockChip::paintEvent(QPaintEvent *e) {
    QLabel::paintEvent(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(pulse_);
    p.setPen(Qt::NoPen);
    const bool light = qApp->palette().color(QPalette::Window).lightness() > 128;
    p.setBrush(QColor(light ? Theme::bright().successText : Theme::dark().successText));
    p.drawEllipse(QRectF(1, height() / 2.0 - 3, 6, 6));
}
PriceLabel::PriceLabel(ThemeManager &theme, QWidget *parent, int pixels)
    : QLabel(parent), theme_(theme), pixels_(pixels) {
    setFont(Fonts::numbers(pixels, QFont::Bold));
    setObjectName("lblCurrentPrice");
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    animation_.setDuration(600);
    connect(&animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        flash_ = value.value<QColor>();
        update();
    });
}
void PriceLabel::setAmount(domain::Money value, bool outbid) {
    if (value_ == value.amount())
        return;
    if (value_) {
        auto c = QColor(outbid ? theme_.theme().danger : theme_.theme().success);
        c.setAlpha(45);
        animation_.stop();
        animation_.setStartValue(c);
        c.setAlpha(0);
        animation_.setEndValue(c);
        animation_.start();
    }
    value_ = value.amount();
    setText(Formatters::formatMoney(value));
    setToolTip(text());
    fitText();
}
void PriceLabel::fitText() {
    int pixels = pixels_;
    while (pixels > 12 &&
           QFontMetrics(Fonts::numbers(pixels, QFont::Bold)).horizontalAdvance(text()) > width())
        --pixels;
    setFont(Fonts::numbers(pixels, QFont::Bold));
}
void PriceLabel::resizeEvent(QResizeEvent *e) {
    QLabel::resizeEvent(e);
    fitText();
}
void PriceLabel::paintEvent(QPaintEvent *event) {
    if (flash_.alpha() > 0) {
        QPainter p(this);
        p.setPen(Qt::NoPen);
        p.setBrush(flash_);
        p.drawRoundedRect(rect(), 8, 8);
    }
    QLabel::paintEvent(event);
}
MoneyLineEdit::MoneyLineEdit(QWidget *parent) : QLineEdit(parent) {
    setFont(Fonts::numbers(18, QFont::DemiBold));
    setAlignment(Qt::AlignRight);
    setTextMargins(0, 0, 44, 0);
    setAccessibleName("Số tiền, Việt Nam đồng");
    setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9.]{0,27}"), this));
    connect(this, &QLineEdit::textEdited, this, &MoneyLineEdit::formatInput);
    setValue(0);
}
qint64 MoneyLineEdit::value() const {
    QString s = text();
    s.remove('.');
    return s.toLongLong();
}
void MoneyLineEdit::setValue(qint64 v) {
    v = std::max<qint64>(0, v);
    const auto s = Formatters::formatMoney(domain::Money(v)).section(' ', 0, 0);
    if (text() == s)
        return;
    setText(s);
    previous_ = s;
    emit valueChanged(v);
}
void MoneyLineEdit::formatInput(const QString &text) {
    int digitsAfter = 0;
    for (int i = cursorPosition(); i < text.size(); ++i)
        if (text[i].isDigit())
            ++digitsAfter;
    QString digits = text;
    digits.remove('.');
    bool ok = false;
    const auto v = digits.isEmpty() ? 0 : digits.toLongLong(&ok);
    if (!digits.isEmpty() && !ok) {
        setText(previous_);
        return;
    }
    setValue(v);
    int pos = this->text().size();
    while (pos > 0 && digitsAfter > 0) {
        --pos;
        if (this->text()[pos].isDigit())
            --digitsAfter;
    }
    setCursorPosition(pos);
}
void MoneyLineEdit::paintEvent(QPaintEvent *event) {
    QLineEdit::paintEvent(event);
    QPainter p(this);
    p.setFont(Fonts::body(12));
    p.setPen(palette().color(QPalette::Text));
    p.drawText(rect().adjusted(0, 0, -12, 0), Qt::AlignRight | Qt::AlignVCenter, "VND");
}
StepIndicator::StepIndicator(const QStringList &steps, QWidget *parent) : QWidget(parent) {
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < steps.size(); ++i) {
        auto *l = label(QString::number(i + 1) + "  " + steps[i], this, 12);
        labels_ << l;
        row->addWidget(l);
    }
    setStep(0);
}
void StepIndicator::setStep(int step) {
    for (int i = 0; i < labels_.size(); ++i)
        tone(labels_[i], i == step ? "accent" : i < step ? "success" : "muted");
}
StatCard::StatCard(const QString &title, const QString &icon, ThemeManager &theme, QWidget *parent)
    : Card(parent) {
    auto *row = new QHBoxLayout;
    auto *text = label(title, this, 12, "muted");
    row->addWidget(text, 1);
    auto *glyph = new QLabel(this);
    row->addWidget(glyph);
    auto refresh = [glyph, icon, &theme] {
        glyph->setPixmap(IconProvider::icon(icon, QColor(theme.theme().accent)).pixmap(20));
    };
    connect(&theme, &ThemeManager::changed, this, refresh);
    refresh();
    content->addLayout(row);
    value = label("0", this, 24);
    value->setFont(Fonts::numbers(24, QFont::DemiBold));
    value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    content->addWidget(value);
}
EmptyState::EmptyState(const QString &title, const QString &description, const QString &actionText,
                       ThemeManager &theme, QWidget *parent, const QString &icon)
    : Card(parent) {
    auto *glyph = new QLabel(this);
    glyph->setAlignment(Qt::AlignCenter);
    auto refresh = [glyph, icon, &theme] {
        glyph->setPixmap(IconProvider::icon(icon, QColor(theme.theme().accent), 40).pixmap(40));
    };
    connect(&theme, &ThemeManager::changed, this, refresh);
    refresh();
    content->addStretch();
    content->addWidget(glyph);
    auto *t = label(title, this, 20);
    t->setAlignment(Qt::AlignCenter);
    content->addWidget(t);
    auto *desc = label(description, this, 14, "muted");
    desc->setAlignment(Qt::AlignCenter);
    content->addWidget(desc);
    action = button(actionText, "emptyAction", this, "primary");
    action->setVisible(!actionText.isEmpty());
    content->addWidget(action, 0, Qt::AlignCenter);
    content->addStretch();
}
ToastManager::ToastManager(UiTicker &ticker, ThemeManager &theme, QWidget *parent)
    : QWidget(parent), ticker_(ticker), theme_(theme) {
    setObjectName("toastHost");
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(8);
    setFixedWidth(410);
    hide();
    connect(&ticker, &UiTicker::tick, this, [this](common::WallTime, common::SteadyTime now) {
        for (int i = toasts_.size() - 1; i >= 0; --i) {
            auto &t = toasts_[i];
            if (t.card && t.card->underMouse())
                t.expiry = now + std::chrono::seconds(4);
            if (!t.card || now >= t.expiry) {
                if (t.card) {
                    layout_->removeWidget(t.card);
                    t.card->deleteLater();
                }
                toasts_.removeAt(i);
            }
        }
        if (toasts_.isEmpty())
            hide();
        else
            reposition();
    });
}
void ToastManager::showToast(const QString &message, const QString &color, const QString &action,
                             std::function<void()> callback) {
    if (toasts_.size() >= 3) {
        auto first = toasts_.takeFirst();
        layout_->removeWidget(first.card);
        delete first.card;
    }
    auto *card = new Card(this, true);
    card->setAccessibleName(message);
    auto *row = new QHBoxLayout;
    auto *msg = label(message, card);
    tone(msg, color);
    row->addWidget(msg, 1);
    auto *close = button("×", "toastClose", card, "ghost");
    close->setAccessibleName("Đóng thông báo");
    close->setToolTip("Đóng thông báo");
    row->addWidget(close, 0, Qt::AlignTop);
    card->content->addLayout(row);
    connect(close, &QPushButton::clicked, card, [card] { card->deleteLater(); });
    if (!action.isEmpty()) {
        auto *b = button(action, "toastAction", card, "ghost");
        card->content->addWidget(b);
        connect(b, &QPushButton::clicked, card, [callback, card] {
            if (callback)
                callback();
            card->deleteLater();
        });
    }
    layout_->addWidget(card);
    toasts_.append({card, ticker_.clock().now() + std::chrono::seconds(4)});
    show();
    reposition();
    raise();
    auto *effect = new QGraphicsOpacityEffect(card);
    card->setGraphicsEffect(effect);
    auto *animation = new QPropertyAnimation(effect, "opacity", card);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(180);
    connect(animation, &QPropertyAnimation::finished, card,
            [card] { card->setGraphicsEffect(nullptr); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
void ToastManager::reposition() {
    adjustSize();
    move(parentWidget()->width() - width() - 24, parentWidget()->height() - height() - 42);
    raise();
}
} // namespace auction::presentation
