#pragma once
#include <QByteArray>
#include <QImage>
#include <QString>
namespace auction::media {
// Local import respects orientation, preserves the full composition and bounds decoding.
QByteArray importProductImage(const QString &path);
// Network images must already be bounded PNG/JPEG. Never load a remote file path.
QImage decodeProductImage(const QByteArray &bytes);
}
