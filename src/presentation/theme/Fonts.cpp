#include "auction/presentation/theme/Fonts.h"

#include <QApplication>
#include <QFontDatabase>
namespace auction::presentation {
void Fonts::install() {
    qApp->setFont(body());
}
QFont Fonts::body(int pixels, int weight) {
    QFont f;
    f.setFamilies({"Inter", "Segoe UI", "SF Pro Text", "Roboto", "sans-serif"});
    f.setPixelSize(pixels);
    f.setWeight(static_cast<QFont::Weight>(weight));
    return f;
}
QFont Fonts::numbers(int pixels, int weight) {
    QFont f = body(pixels, weight);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    f.setFeature(QFont::Tag("tnum"), 1);
#else
    f.setFamilies({"Consolas", "Menlo", "DejaVu Sans Mono"});
#endif
    return f;
}
} // namespace auction::presentation
