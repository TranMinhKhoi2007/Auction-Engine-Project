#include "auction/presentation/theme/StyleSheetBuilder.h"

#include <QFile>
namespace auction::presentation {
QString StyleSheetBuilder::build(const Theme &t) {
    QFile file(":/app.qss.in");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QString qss = QString::fromUtf8(file.readAll());
#define TOKEN(n) qss.replace("@" #n "@", t.n)
    TOKEN(bgApp);
    TOKEN(bgSurface);
    TOKEN(bgElevated);
    TOKEN(bgInput);
    TOKEN(border);
    TOKEN(borderStrong);
    TOKEN(text);
    TOKEN(textMuted);
    TOKEN(textDisabled);
    TOKEN(accent);
    TOKEN(accentHover);
    TOKEN(accentPress);
    TOKEN(onAccent);
    TOKEN(successText);
    TOKEN(warningText);
    TOKEN(dangerText);
    TOKEN(infoText);
#undef TOKEN
    return qss;
}
} // namespace auction::presentation
