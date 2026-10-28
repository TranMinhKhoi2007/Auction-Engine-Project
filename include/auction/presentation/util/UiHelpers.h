#pragma once
#include "auction/presentation/theme/Fonts.h"

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>
#include <functional>
namespace auction::presentation {
inline void tone(QWidget *widget, const QString &value) {
    if (widget->property("tone") == value)
        return;
    widget->setProperty("tone", value);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}
inline QLabel *label(const QString &text, QWidget *parent, int pixels = 14,
                     const QString &color = {}) {
    auto *l = new QLabel(text, parent);
    l->setTextFormat(Qt::PlainText);
    l->setWordWrap(true);
    l->setFont(Fonts::body(pixels, pixels >= 16 ? QFont::DemiBold : QFont::Normal));
    l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    if (!color.isEmpty())
        tone(l, color);
    return l;
}
inline QPushButton *button(const QString &text, const QString &name, QWidget *parent,
                           const QString &variant = "secondary") {
    auto *b = new QPushButton(text, parent);
    b->setObjectName(name);
    b->setAccessibleName(text);
    b->setProperty("variant", variant);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}
inline QScrollArea *scrollPage(QWidget *body, QWidget *parent) {
    auto *s = new QScrollArea(parent);
    s->setWidgetResizable(true);
    s->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s->setWidget(body);
    return s;
}
inline QString titleCase(QString s) {
    s = s.simplified();
    bool start = true;
    for (auto &c : s) {
        if (c.isSpace()) {
            start = true;
        } else if (start) {
            c = c.toUpper();
            start = false;
        }
    }
    return s;
}
template <class F, class E>
bool runGuarded(F &&action, E &&error) {
    try {
        action();
        return true;
    } catch (const std::exception &e) {
        error(QString::fromUtf8(e.what()));
    } catch (...) {
        error(QStringLiteral("Không thể hoàn tất thao tác. Vui lòng thử lại."));
    }
    return false;
}
} // namespace auction::presentation
