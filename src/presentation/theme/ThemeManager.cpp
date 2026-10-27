#include "auction/presentation/theme/ThemeManager.h"

#include "auction/presentation/theme/Fonts.h"
#include "auction/presentation/theme/StyleSheetBuilder.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
static void initResources() {
    Q_INIT_RESOURCE(app);
}
namespace auction::presentation {
ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {
    initResources();
    theme_ = QSettings("AuctionEngine", "Presentation").value("light", false).toBool()
                 ? Theme::bright()
                 : Theme::dark();
    qApp->setStyle(QStyleFactory::create("Fusion"));
    Fonts::install();
    apply();
}
void ThemeManager::setLight(bool light) {
    theme_ = light ? Theme::bright() : Theme::dark();
    QSettings("AuctionEngine", "Presentation").setValue("light", light);
    apply();
    emit changed();
}
void ThemeManager::apply() {
    QPalette p;
    p.setColor(QPalette::Window, QColor(theme_.bgApp));
    p.setColor(QPalette::Base, QColor(theme_.bgInput));
    p.setColor(QPalette::AlternateBase, QColor(theme_.bgElevated));
    p.setColor(QPalette::WindowText, QColor(theme_.text));
    p.setColor(QPalette::Text, QColor(theme_.text));
    p.setColor(QPalette::PlaceholderText, QColor(theme_.textMuted));
    p.setColor(QPalette::Button, QColor(theme_.bgSurface));
    p.setColor(QPalette::ButtonText, QColor(theme_.text));
    p.setColor(QPalette::Highlight, QColor(theme_.accent));
    p.setColor(QPalette::HighlightedText, QColor(theme_.onAccent));
    p.setColor(QPalette::ToolTipBase, QColor(theme_.bgElevated));
    p.setColor(QPalette::ToolTipText, QColor(theme_.text));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(theme_.textDisabled));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(theme_.textDisabled));
    qApp->setPalette(p);
    qApp->setStyleSheet(StyleSheetBuilder::build(theme_));
}
} // namespace auction::presentation
