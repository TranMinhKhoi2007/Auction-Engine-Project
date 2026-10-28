#pragma once
#include <QMap>
#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QSet>
#include "auction/client/IAuctionGateway.h"
namespace auction::presentation {
class ProductImageStore : public QObject {
    Q_OBJECT
public:
    explicit ProductImageStore(QObject *parent = nullptr) : QObject(parent) {}
    bool setImage(quint64 id, const QString &path);
    void attachGateway(client::IAuctionGateway &gateway);
    void ensureImage(quint64 id, bool hasImage);
    QPixmap image(quint64 id, const QString &category, QSize size, qreal dpr = 1) const;
    [[nodiscard]] bool hasLoadedImage(quint64 id) const { return images_.contains(id); }
    [[nodiscard]] QImage rawImage(quint64 id) const { return images_.value(id); }
signals:
    void changed(quint64 id);
    void loadFailed(quint64 id, const QString &message);

private:
    QMap<quint64, QImage> images_;
    void fetchNext();
    client::IAuctionGateway *gateway_{};
    QSet<quint64> requested_;
    QQueue<quint64> queue_;
    quint64 generation_{};
    bool fetching_{};
};
} // namespace auction::presentation
