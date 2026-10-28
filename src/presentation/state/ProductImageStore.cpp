#include "auction/presentation/state/ProductImageStore.h"

#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/theme/Theme.h"
#include "auction/media/ProductImage.h"
#include "auction/protocol/JsonCodec.h"

#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
namespace auction::presentation {
bool ProductImageStore::setImage(quint64 id, const QString &path) {
    try {images_[id] = media::decodeProductImage(media::importProductImage(path));}
    catch(const std::exception &) {return false;}
    emit changed(id);
    return true;
}
void ProductImageStore::attachGateway(client::IAuctionGateway &gateway) {
    Q_ASSERT(!gateway_);gateway_=&gateway;
    connect(&gateway,&client::IAuctionGateway::connectionStateChanged,this,[this](client::ConnectionState state) {
        if(state!=client::ConnectionState::Connected)return;
        ++generation_;queue_.clear();requested_.clear();fetching_=false;
        const auto ids=images_.keys();images_.clear();for(auto id:ids)emit changed(id);
    });
}
void ProductImageStore::ensureImage(quint64 id,bool hasImage) {
    if(!hasImage || !gateway_ || images_.contains(id) || requested_.contains(id))return;
    requested_.insert(id);queue_.enqueue(id);fetchNext();
}
void ProductImageStore::fetchNext() {
    if(fetching_ || queue_.isEmpty() || !gateway_ || gateway_->state()!=client::ConnectionState::Connected)return;
    const auto id=queue_.dequeue();const auto generation=generation_;fetching_=true;
    // One image at a time avoids filling the socket queue when a whole grid loads.
    gateway_->request("auction.image",{{"auctionId",double(id)}},this,[this,id,generation](auto result) {
        if(generation!=generation_)return;
        fetching_=false;
        try {
            if(!result)throw std::runtime_error(result.error->message.toStdString());
            const auto bytes=protocol::readImageData(protocol::readObject(*result.value).value("imageBase64"));
            images_[id]=media::decodeProductImage(QByteArray::fromStdString(bytes));emit changed(id);
        } catch(const std::exception &error) {
            requested_.remove(id);emit loadFailed(id,QString::fromUtf8(error.what()));
        }
        fetchNext();
    });
}
QPixmap ProductImageStore::image(quint64 id, const QString &category, QSize size, qreal dpr) const {
    QPixmap result(size * dpr);
    result.setDevicePixelRatio(dpr);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(QPointF(0, 0), QSizeF(size)), 12, 12);
    p.setClipPath(clip);
    if (images_.contains(id)) {
        const auto &im=images_[id];
        p.fillRect(QRect(QPoint(),size),QColor(128,128,128,20));
        const QSizeF available(qMax(1,size.width()-16),qMax(1,size.height()-16));
        const auto fitted=QSizeF(im.size()).scaled(available,Qt::KeepAspectRatio);
        const QRectF target(QPointF((size.width()-fitted.width())/2,(size.height()-fitted.height())/2),fitted);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawImage(target,im);
        return result;
    }
    auto colors = Theme::categoryColors(category);
    QLinearGradient gradient(0, size.height(), size.width(), 0);
    gradient.setColorAt(0, colors.first);
    gradient.setColorAt(1, colors.second);
    p.fillRect(QRect(QPoint(), size), gradient);
    p.setPen(QPen(QColor(255, 255, 255, 20), 1));
    const int center = size.width() / 2;
    for (int radius = 50; radius < 300; radius += 40)
        p.drawEllipse(QPoint(center, size.height() / 2), radius, radius);
    const auto icon = category.contains("Tranh")   ? "frame"
                      : category.contains("Thiết") ? "cpu"
                                                   : "vase";
    int side = qMin(size.height() * 3 / 5, 170);
    auto pix = IconProvider::icon(icon, QColor(255, 240, 212, 190), side, dpr)
                   .pixmap(QSize(side, side), dpr);
    p.drawPixmap((size.width() - side) / 2, (size.height() - side) / 2, pix);
    return result;
}
} // namespace auction::presentation
