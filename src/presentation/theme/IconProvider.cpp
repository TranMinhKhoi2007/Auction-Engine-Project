#include "auction/presentation/theme/IconProvider.h"

#include <QApplication>
#include <QPainter>
#include <QPixmapCache>
#include <QSvgRenderer>
namespace auction::presentation {
QIcon IconProvider::icon(const QString &name, const QColor &color, int size, qreal dpr) {
    if (dpr <= 0)
        dpr = qApp->devicePixelRatio();
    const auto key =
        QString("auction/%1/%2/%3/%4").arg(name, color.name(QColor::HexArgb)).arg(size).arg(dpr);
    QPixmap pix;
    if (!QPixmapCache::find(key, &pix)) {
        pix = QPixmap(qRound(size * dpr), qRound(size * dpr));
        pix.setDevicePixelRatio(dpr);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        QSvgRenderer svg(":/icons/" + name + ".svg");
        // Paint in logical coordinates: the pixmap's DPR already scales the painter.
        const QRectF bounds(0, 0, size, size);
        svg.render(&p, bounds);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(bounds, color);
        p.end();
        QPixmapCache::insert(key, pix);
    }
    return QIcon(pix);
}
} // namespace auction::presentation
