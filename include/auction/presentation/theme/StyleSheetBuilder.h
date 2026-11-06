#pragma once
#include "auction/presentation/theme/Theme.h"
namespace auction::presentation {
struct StyleSheetBuilder {
    static QString build(const Theme &theme);
};
} // namespace auction::presentation
