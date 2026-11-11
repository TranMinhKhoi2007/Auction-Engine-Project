#include "auction/media/ProductImage.h"
#include "auction/common/ImageLimits.h"
#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <stdexcept>
namespace auction::media {
namespace {
[[noreturn]] void invalid() {throw std::runtime_error("Ảnh không hợp lệ hoặc quá lớn. Hãy chọn ảnh PNG/JPG/WebP/BMP khác.");}
bool bounded(QSize size,int side,qint64 pixels) {
    return size.isValid() && size.width()<=side && size.height()<=side && qint64(size.width())*size.height()<=pixels;
}
}
QImage decodeProductImage(const QByteArray &bytes) {
    if(bytes.isEmpty() || bytes.size()>static_cast<qsizetype>(common::kMaxProductImageBytes))invalid();
    QBuffer buffer;buffer.setData(bytes);buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const auto format=reader.format();
    if(format!="png" && format!="jpeg" && format!="jpg")invalid();
    if(!bounded(reader.size(),common::kMaxProductImageSide,qint64(common::kMaxProductImageSide)*common::kMaxProductImageSide))invalid();
    auto image=reader.read();if(image.isNull())invalid();return image;
}
QByteArray importProductImage(const QString &path) {
    const QFileInfo file(path);
    if(!file.isFile() || file.size()>32*1024*1024)invalid();
    QImageReader reader(path);reader.setAutoTransform(true);
    const auto format=reader.format();
    if(format!="png" && format!="jpeg" && format!="jpg" && format!="webp" && format!="bmp")invalid();
    if(!bounded(reader.size(),20000,50'000'000))invalid();
    if(reader.size().width()>common::kMaxProductImageSide || reader.size().height()>common::kMaxProductImageSide)
        reader.setScaledSize(reader.size().scaled(common::kMaxProductImageSide,common::kMaxProductImageSide,Qt::KeepAspectRatio));
    auto image=reader.read();if(image.isNull())invalid();
    if(image.width()>common::kMaxProductImageSide || image.height()>common::kMaxProductImageSide)
        image=image.scaled(common::kMaxProductImageSide,common::kMaxProductImageSide,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    for(int attempt=0;attempt<24;++attempt) {
        QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);
        if(!image.save(&buffer,"PNG"))invalid();
        if(bytes.size()<=static_cast<qsizetype>(common::kMaxProductImageBytes))return bytes;
        if(!image.hasAlphaChannel()) {
            for(int quality:{90,80,70}) {
                bytes.clear();buffer.seek(0);
                if(!image.save(&buffer,"JPEG",quality))invalid();
                if(bytes.size()<=static_cast<qsizetype>(common::kMaxProductImageBytes))return bytes;
            }
        }
        image=image.scaled(qMax(1,image.width()*4/5),qMax(1,image.height()*4/5),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    }
    invalid();
}
}
