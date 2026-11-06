#pragma once

#include "auction/presentation/theme/ThemeManager.h"

#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVector>

namespace auction::presentation {

class StateLifecycleWidget : public QFrame {
    Q_OBJECT
public:
    explicit StateLifecycleWidget(ThemeManager &theme, QWidget *parent = nullptr);
    void setState(const QString &stateName);

private:
    struct StepUi {
        QFrame *card{nullptr};
        QLabel *stepNum{nullptr};
        QLabel *titleLabel{nullptr};
        QLabel *classBadge{nullptr};
        QLabel *statusLabel{nullptr};
        QLabel *arrowLabel{nullptr};
        QString stateName;
        QString className;
    };

    void refreshSteps();

    ThemeManager &theme_;
    QString currentState_;
    QLabel *headerTitle_{nullptr};
    QLabel *activeClassBadge_{nullptr};
    QLabel *behaviorDescription_{nullptr};
    QVector<StepUi> steps_;
};

} // namespace auction::presentation
