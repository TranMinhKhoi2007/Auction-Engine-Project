#pragma once
#include "auction/domain/ports/IClock.h"

#include <QObject>
#include <QTimer>
namespace auction::presentation {
class UiTicker : public QObject {
    Q_OBJECT
public:
    UiTicker(const domain::IClock &clock, QObject *parent = nullptr);
    const domain::IClock &clock() const { return clock_; }
signals:
    void tick(common::WallTime wall, common::SteadyTime steady);

private:
    const domain::IClock &clock_;
    QTimer timer_;
};
} // namespace auction::presentation
