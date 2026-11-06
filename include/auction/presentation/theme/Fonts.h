#pragma once
#include <QFont>
namespace auction::presentation {
struct Fonts {
    static void install();
    static QFont body(int pixels = 14, int weight = QFont::Normal);
    static QFont numbers(int pixels = 14, int weight = QFont::Normal);
};
} // namespace auction::presentation
