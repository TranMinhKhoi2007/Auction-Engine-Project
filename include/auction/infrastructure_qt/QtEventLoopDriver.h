#pragma once

#include "auction/domain/ports/IClock.h"
#include "auction/infrastructure/EventLoop.h"
#include <QObject>
#include <QTimer>

namespace auction::infrastructure_qt {

class QtEventLoopDriver final : public QObject {
    Q_OBJECT

public:
    QtEventLoopDriver(infrastructure::EventLoop& eventLoop,
                      const domain::IClock& clock,
                      QObject* parent = nullptr);
    ~QtEventLoopDriver() override;

    void start();
    void stop();

private slots:
    void onTimeout();

private:
    void rearm();

    infrastructure::EventLoop& eventLoop_;
    const domain::IClock& clock_;
    QTimer timer_;
    bool isProcessing_{false};
    bool running_{false};
    bool needsRearm_{false};
};

} // namespace auction::infrastructure_qt
