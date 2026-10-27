#pragma once
#include <QSslConfiguration>
#include <QString>
#include <QUrl>
namespace auction::transport {
QSslConfiguration serverTls(const QString &certificateFile,const QString &keyFile);
QSslConfiguration clientTls(const QString &caFile={});
bool validServerUrl(const QUrl &url);
}
