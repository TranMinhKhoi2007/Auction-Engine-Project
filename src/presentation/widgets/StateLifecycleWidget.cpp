#include "auction/presentation/widgets/StateLifecycleWidget.h"

#include "auction/presentation/theme/Fonts.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QGraphicsDropShadowEffect>

namespace auction::presentation {

StateLifecycleWidget::StateLifecycleWidget(ThemeManager &theme, QWidget *parent)
    : QFrame(parent), theme_(theme) {
    setObjectName("stateLifecycleWidget");
    setProperty("card", true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(10);

    // Header: Tiêu đề State Pattern & Badge lớp C++ OOP đang hoạt động
    auto *headerLayout = new QHBoxLayout;
    headerTitle_ = label("MÔ HÌNH TRẠNG THÁI (STATE PATTERN OOP)", this, 13);
    headerTitle_->setFont(Fonts::body(13, QFont::Bold));
    headerTitle_->setStyleSheet("letter-spacing: 0.5px;");
    headerLayout->addWidget(headerTitle_);

    headerLayout->addStretch();

    activeClassBadge_ = label("IAuctionState: Chưa nạp", this, 12);
    activeClassBadge_->setStyleSheet(
        "padding: 3px 8px; border-radius: 6px; font-family: monospace; font-weight: bold;");
    headerLayout->addWidget(activeClassBadge_);

    mainLayout->addLayout(headerLayout);

    // Pipeline Layout: Các bước trạng thái nối tiếp nhau
    auto *pipelineLayout = new QHBoxLayout;
    pipelineLayout->setSpacing(6);

    struct StepDef {
        QString stateName;
        QString className;
    };

    const QVector<StepDef> stepDefs = {
        {"Chờ duyệt", "ChoDuyetState"},
        {"Sắp diễn ra", "SapDienRaState"},
        {"Đang diễn ra", "DangDienRaState"},
        {"Kết thúc", "KetThucState"}
    };

    for (int i = 0; i < stepDefs.size(); ++i) {
        StepUi ui;
        ui.stateName = stepDefs[i].stateName;
        ui.className = stepDefs[i].className;

        ui.card = new QFrame(this);
        ui.card->setObjectName("stateStepCard_" + QString::number(i));
        auto *cardLayout = new QVBoxLayout(ui.card);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(3);

        auto *topRow = new QHBoxLayout;
        ui.stepNum = label(QString::number(i + 1), ui.card, 11);
        ui.stepNum->setFixedSize(20, 20);
        ui.stepNum->setAlignment(Qt::AlignCenter);
        ui.stepNum->setStyleSheet("border-radius: 10px; font-weight: bold;");
        topRow->addWidget(ui.stepNum);

        ui.titleLabel = label(ui.stateName, ui.card, 13);
        ui.titleLabel->setFont(Fonts::body(13, QFont::DemiBold));
        topRow->addWidget(ui.titleLabel, 1);
        cardLayout->addLayout(topRow);

        ui.classBadge = label(ui.className, ui.card, 11);
        ui.classBadge->setStyleSheet(
            "font-family: monospace; font-size: 11px; padding: 1px 4px; border-radius: 4px;");
        cardLayout->addWidget(ui.classBadge);

        ui.statusLabel = label("○ Chờ tới", ui.card, 11, "muted");
        cardLayout->addWidget(ui.statusLabel);

        pipelineLayout->addWidget(ui.card, 1);

        // Mũi tên chuyển trạng thái (trừ bước cuối)
        if (i < stepDefs.size() - 1) {
            ui.arrowLabel = label("➔", this, 14, "muted");
            ui.arrowLabel->setAlignment(Qt::AlignCenter);
            ui.arrowLabel->setFixedWidth(16);
            pipelineLayout->addWidget(ui.arrowLabel);
        }

        steps_.append(ui);
    }

    mainLayout->addLayout(pipelineLayout);

    // Callout mô tả quy tắc xử lý của State OOP hiện tại
    behaviorDescription_ = label("", this, 12, "muted");
    behaviorDescription_->setObjectName("statePatternBehaviorDesc");
    behaviorDescription_->setWordWrap(true);
    behaviorDescription_->setStyleSheet(
        "padding: 6px 10px; border-radius: 6px; background: rgba(127, 127, 127, 0.08);");
    mainLayout->addWidget(behaviorDescription_);

    connect(&theme_, &ThemeManager::changed, this, &StateLifecycleWidget::refreshSteps);
}

void StateLifecycleWidget::setState(const QString &stateName) {
    currentState_ = stateName;
    refreshSteps();
}

void StateLifecycleWidget::refreshSteps() {
    const auto &theme = theme_.theme();
    const bool isDark = !theme.light;

    int activeIdx = -1;
    if (currentState_ == "Chờ duyệt") activeIdx = 0;
    else if (currentState_ == "Sắp diễn ra") activeIdx = 1;
    else if (currentState_ == "Đang diễn ra") activeIdx = 2;
    else if (currentState_ == "Kết thúc") activeIdx = 3;
    else if (currentState_ == "Hủy") activeIdx = 4; // Đặc biệt

    // Cập nhật bước cuối nếu phiên bị hủy
    if (steps_.size() >= 4) {
        if (activeIdx == 4) {
            steps_[3].stateName = "Đã hủy";
            steps_[3].className = "HuyState";
            steps_[3].titleLabel->setText("Đã hủy");
            steps_[3].classBadge->setText("HuyState");
        } else {
            steps_[3].stateName = "Kết thúc";
            steps_[3].className = "KetThucState";
            steps_[3].titleLabel->setText("Kết thúc");
            steps_[3].classBadge->setText("KetThucState");
        }
    }

    // Cập nhật huy hiệu góc trên
    QString activeClassName = "Chưa rõ";
    if (activeIdx >= 0 && activeIdx < 4) {
        activeClassName = steps_[activeIdx].className;
    } else if (activeIdx == 4) {
        activeClassName = "HuyState";
    }

    activeClassBadge_->setText("● ĐANG THỰC THI: " + activeClassName);
    if (activeIdx == 2) {
        // Đang diễn ra (Live)
        activeClassBadge_->setStyleSheet(
            "padding: 3px 10px; border-radius: 6px; font-family: monospace; font-weight: bold; "
            "background: rgba(34, 197, 94, 0.18); color: #22c55e; border: 1px solid #22c55e;");
    } else if (activeIdx == 0 || activeIdx == 1) {
        activeClassBadge_->setStyleSheet(
            "padding: 3px 10px; border-radius: 6px; font-family: monospace; font-weight: bold; "
            "background: rgba(59, 130, 246, 0.18); color: #3b82f6; border: 1px solid #3b82f6;");
    } else if (activeIdx == 4) {
        activeClassBadge_->setStyleSheet(
            "padding: 3px 10px; border-radius: 6px; font-family: monospace; font-weight: bold; "
            "background: rgba(239, 68, 68, 0.18); color: #ef4444; border: 1px solid #ef4444;");
    } else {
        activeClassBadge_->setStyleSheet(
            "padding: 3px 10px; border-radius: 6px; font-family: monospace; font-weight: bold; "
            "background: rgba(168, 85, 247, 0.18); color: #a855f7; border: 1px solid #a855f7;");
    }

    // Cập nhật từng step card
    for (int i = 0; i < steps_.size(); ++i) {
        auto &s = steps_[i];
        if (activeIdx == 4) {
            // Trường hợp Hủy
            if (i < 3) {
                s.stepNum->setText("✕");
                s.stepNum->setStyleSheet("border-radius: 10px; font-weight: bold; background: rgba(127,127,127,0.2); color: " + theme.textMuted + ";");
                s.card->setStyleSheet("QFrame#stateStepCard_" + QString::number(i) + " { border: 1px solid " + theme.border + "; border-radius: 8px; background: transparent; opacity: 0.6; }");
                s.statusLabel->setText("✕ Đã dừng");
                s.classBadge->setStyleSheet("font-family: monospace; font-size: 11px; padding: 1px 4px; border-radius: 4px; background: rgba(127,127,127,0.1); color: " + theme.textMuted + ";");
            } else {
                s.stepNum->setText("!");
                s.stepNum->setStyleSheet("border-radius: 10px; font-weight: bold; background: #ef4444; color: white;");
                s.card->setStyleSheet("QFrame#stateStepCard_" + QString::number(i) + " { border: 2px solid #ef4444; border-radius: 8px; background: rgba(239, 68, 68, 0.12); }");
                s.statusLabel->setText("● Đã hủy phiên");
                s.statusLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
                s.classBadge->setStyleSheet("font-family: monospace; font-size: 11px; padding: 1px 4px; border-radius: 4px; background: rgba(239, 68, 68, 0.2); color: #ef4444; font-weight: bold;");
            }
        } else if (i < activeIdx) {
            // Đã vượt qua (Past)
            s.stepNum->setText("✓");
            s.stepNum->setStyleSheet("border-radius: 10px; font-weight: bold; background: rgba(34, 197, 94, 0.2); color: #22c55e;");
            s.card->setStyleSheet("QFrame#stateStepCard_" + QString::number(i) + " { border: 1px solid " + theme.border + "; border-radius: 8px; background: transparent; }");
            s.statusLabel->setText("✓ Hoàn thành");
            s.statusLabel->setStyleSheet("color: #22c55e; font-size: 11px;");
            s.classBadge->setStyleSheet("font-family: monospace; font-size: 11px; padding: 1px 4px; border-radius: 4px; background: rgba(127,127,127,0.1); color: " + theme.textMuted + ";");
        } else if (i == activeIdx) {
            // Đang kích hoạt (Active)
            QString highlightColor = (i == 2) ? "#22c55e" : (i == 3) ? "#a855f7" : "#3b82f6";
            s.stepNum->setText(QString::number(i + 1));
            s.stepNum->setStyleSheet("border-radius: 10px; font-weight: bold; background: " + highlightColor + "; color: white;");
            s.card->setStyleSheet("QFrame#stateStepCard_" + QString::number(i) + " { border: 2px solid " + highlightColor + "; border-radius: 8px; background: " + (isDark ? "rgba(255,255,255,0.06)" : "rgba(0,0,0,0.03)") + "; }");
            s.statusLabel->setText("● Đang kích hoạt");
            s.statusLabel->setStyleSheet("color: " + highlightColor + "; font-weight: bold; font-size: 11px;");
            s.classBadge->setStyleSheet("font-family: monospace; font-size: 11px; padding: 1px 4px; border-radius: 4px; background: " + highlightColor + "; color: white; font-weight: bold;");
        } else {
            // Sắp tới (Upcoming)
            s.stepNum->setText(QString::number(i + 1));
            s.stepNum->setStyleSheet("border-radius: 10px; font-weight: bold; background: rgba(127,127,127,0.2); color: " + theme.textMuted + ";");
            s.card->setStyleSheet("QFrame#stateStepCard_" + QString::number(i) + " { border: 1px solid " + theme.border + "; border-radius: 8px; background: transparent; }");
            s.statusLabel->setText("○ Chờ tới");
            s.statusLabel->setStyleSheet("color: " + theme.textMuted + "; font-size: 11px;");
            s.classBadge->setStyleSheet("font-family: monospace; font-size: 11px; padding: 1px 4px; border-radius: 4px; background: rgba(127,127,127,0.1); color: " + theme.textMuted + ";");
        }
    }

    // Mô tả hành vi State Pattern tương ứng
    if (activeIdx == 0) {
        behaviorDescription_->setText(
            "💡 [ChoDuyetState]: Trạng thái khởi tạo. Chỉ chấp nhận approve() từ Chủ sàn hoặc cancel() từ Người bán. "
            "Toàn bộ yêu cầu đặt cọc (join) và trả giá (placeBid) bị từ chối.");
    } else if (activeIdx == 1) {
        behaviorDescription_->setText(
            "💡 [SapDienRaState]: Chờ đếm ngược đến giờ mở phiên. Cho phép người tham gia đăng ký và ký quỹ đặt cọc (join). "
            "Lệnh đặt giá (placeBid) bị từ chối cho đến khi thời gian bắt đầu điểm.");
    } else if (activeIdx == 2) {
        behaviorDescription_->setText(
            "💡 [DangDienRaState]: Phiên đấu giá thời gian thực. Tiếp nhận lệnh placeBid(), tự động kiểm tra bước giá, "
            "cập nhật người dẫn đầu và kích hoạt đếm ngược 10 giây kịch tính (Anti-sniping auto-extend).");
    } else if (activeIdx == 3) {
        behaviorDescription_->setText(
            "💡 [KetThucState]: Khóa toàn bộ lệnh đặt giá. Kích hoạt tiếng búa gõ chốt phiên (Gavel Strike), "
            "xác định người thắng chung cuộc, chuyển giao quyền sở hữu vật phẩm vào Kho lưu trữ và quyết toán số dư.");
    } else if (activeIdx == 4) {
        behaviorDescription_->setText(
            "💡 [HuyState]: Phiên đấu giá đã bị dừng hủy. Toàn bộ thao tác bị khóa, hệ thống tự động hoàn trả 100% "
            "tiền ký quỹ đặt cọc về ví cho tất cả người tham gia.");
    } else {
        behaviorDescription_->setText("💡 Mô hình State Pattern điều phối toàn bộ vòng đời phiên đấu giá một cách tường minh.");
    }
}

} // namespace auction::presentation
