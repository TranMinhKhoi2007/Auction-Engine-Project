#pragma once
#include <QColor>
#include <QString>
#include <QStringList>
namespace auction::presentation {
struct Theme {
    QString bgApp, bgSurface, bgElevated, bgInput, border, borderStrong;
    QString text, textMuted, textDisabled, accent, accentHover, accentPress, onAccent;
    QString success, warning, danger, info, neutral;
    QString successText, warningText, dangerText, infoText;
    bool light{false};
    static Theme dark();
    static Theme bright();
    static constexpr int space4 = 4, space8 = 8, space12 = 12, space16 = 16, space24 = 24,
                         space32 = 32;
    static constexpr int cardRadius = 12, controlRadius = 8;
    static QStringList avatarColors();
    static QPair<QColor, QColor> categoryColors(const QString &category);
};
} // namespace auction::presentation
