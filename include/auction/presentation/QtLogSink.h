#pragma once

#include <QObject>
#include <QString>
#include <string_view>

namespace auction::presentation {

class QtLogSink final : public QObject {
    Q_OBJECT

public:
    explicit QtLogSink(QObject *parent = nullptr);

    void debug(std::string_view message);
    void info(std::string_view message);
    void warn(std::string_view message);
    void error(std::string_view message);

signals:
    void logReceived(const QString &level, const QString &message);
};

} // namespace auction::presentation
