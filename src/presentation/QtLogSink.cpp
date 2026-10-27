#include "auction/presentation/QtLogSink.h"

namespace auction::presentation {

QtLogSink::QtLogSink(QObject *parent) : QObject(parent) {}

void QtLogSink::debug(std::string_view message) {
    emit logReceived("DEBUG",
                     QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
}

void QtLogSink::info(std::string_view message) {
    emit logReceived("INFO",
                     QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
}

void QtLogSink::warn(std::string_view message) {
    emit logReceived("WARN",
                     QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
}

void QtLogSink::error(std::string_view message) {
    emit logReceived("ERROR",
                     QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
}

} // namespace auction::presentation
