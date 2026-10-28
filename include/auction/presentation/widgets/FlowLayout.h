#pragma once
#include <QLayout>
namespace auction::presentation {
class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget *parent = nullptr, int spacing = 16);
    ~FlowLayout() override;
    void addItem(QLayoutItem *item) override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    Qt::Orientations expandingDirections() const override { return Qt::Horizontal; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;
    void setGeometry(const QRect &rect) override;
    void setListMode(bool value) {
        list_ = value;
        invalidate();
    }

private:
    int arrange(const QRect &rect, bool apply) const;
    QList<QLayoutItem *> items_;
    bool list_{false};
};
} // namespace auction::presentation
