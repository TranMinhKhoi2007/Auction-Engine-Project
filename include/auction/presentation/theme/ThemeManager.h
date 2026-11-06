#pragma once
#include "auction/presentation/theme/Theme.h"

#include <QObject>
namespace auction::presentation {
class ThemeManager : public QObject {
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);
    const Theme &theme() const { return theme_; }
    void setLight(bool light);
    void apply();
signals:
    void changed();

private:
    Theme theme_;
};
} // namespace auction::presentation
