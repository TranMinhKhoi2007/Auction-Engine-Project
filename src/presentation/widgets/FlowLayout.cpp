#include "auction/presentation/widgets/FlowLayout.h"

#include <QWidget>
namespace auction::presentation {
FlowLayout::FlowLayout(QWidget *parent, int gap) : QLayout(parent) {
    setSpacing(gap);
    setContentsMargins(0, 0, 0, 0);
}
FlowLayout::~FlowLayout() {
    while (auto *item = takeAt(0))
        delete item;
}
void FlowLayout::addItem(QLayoutItem *item) {
    items_ << item;
    invalidate();
}
int FlowLayout::count() const {
    return items_.size();
}
QLayoutItem *FlowLayout::itemAt(int i) const {
    return items_.value(i);
}
QLayoutItem *FlowLayout::takeAt(int i) {
    return i >= 0 && i < items_.size() ? items_.takeAt(i) : nullptr;
}
int FlowLayout::heightForWidth(int width) const {
    return arrange(QRect(0, 0, width, 0), false);
}
QSize FlowLayout::minimumSize() const {
    return {280, 0};
}
QSize FlowLayout::sizeHint() const {
    return {600, heightForWidth(600)};
}
void FlowLayout::setGeometry(const QRect &rect) {
    QLayout::setGeometry(rect);
    arrange(rect, true);
}
int FlowLayout::arrange(const QRect &r, bool apply) const {
    int cols = list_ ? 1 : std::max(1, (r.width() + spacing()) / (280 + spacing()));
    const int width = (r.width() - (cols - 1) * spacing()) / cols;
    int col = 0, y = r.y(), height = 0;
    bool any = false;
    for (auto *item : items_) {
        if (item->widget()->isHidden())
            continue;
        any = true;
        int h = item->hasHeightForWidth() ? item->heightForWidth(width) : item->sizeHint().height();
        h = std::max(h, item->minimumSize().height());
        if (col == cols) {
            y += height + spacing();
            height = 0;
            col = 0;
        }
        if (apply)
            item->setGeometry(QRect(r.x() + col * (width + spacing()), y, width, h));
        height = std::max(height, h);
        ++col;
    }
    return any ? y - r.y() + height : 0;
}
} // namespace auction::presentation
