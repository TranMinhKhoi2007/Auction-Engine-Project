#pragma once
#include <QColor>
#include <QIcon>
namespace auction::presentation {
struct IconProvider {
    static QIcon icon(const QString &name, const QColor &color, int size = 20, qreal dpr = 0);
};
} // namespace auction::presentation
