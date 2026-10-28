#include "auction/presentation/theme/Theme.h"
namespace auction::presentation {
Theme Theme::dark() {
    return {"#0B1220", "#111A2E", "#17223B", "#0E1729", "#26324D", "#687A9E", "#E8ECF4", "#9AA7BF",
            "#9AA7BF", "#D4A74A", "#E3BA62", "#B98F38", "#0B1220", "#22C55E", "#F59E0B", "#EF4444",
            "#38BDF8", "#64748B", "#4ADE80", "#FBBF24", "#F87171", "#7DD3FC", false};
}
Theme Theme::bright() {
    return {"#F5F7FB", "#FFFFFF", "#F1F5F9", "#FFFFFF", "#E2E8F0", "#72819A", "#0F172A", "#475569",
            "#475569", "#9A6412", "#85550D", "#744909", "#FFFFFF", "#16803B", "#A35C00", "#C62828",
            "#0369A1", "#64748B", "#166534", "#854D0E", "#B91C1C", "#075985", true};
}
QStringList Theme::avatarColors() {
    return {"#305B82", "#65508A", "#875D32", "#367060", "#7D4563", "#4A608A", "#686235", "#70545C"};
}
QPair<QColor, QColor> Theme::categoryColors(const QString &category) {
    if (category.contains("Tranh"))
        return {QColor("#302342"), QColor("#785069")};
    if (category.contains("Thiết"))
        return {QColor("#132F48"), QColor("#414D81")};
    return {QColor("#302820"), QColor("#80603A")};
}
} // namespace auction::presentation
