#include "auction/presentation/dialogs/Dialogs.h"

#include "auction/domain/AuctionConfig.h"
#include "auction/domain/values/SecretBuffer.h"
#include "auction/presentation/Formatters.h"
#include "auction/presentation/theme/IconProvider.h"
#include "auction/presentation/util/UiHelpers.h"

#include <QButtonGroup>
#include <QDate>
#include <QFileDialog>
#include <QRegularExpression>
#include <QTabWidget>
#include <QStylePainter>
namespace auction::presentation {
namespace {
QLineEdit *input(QWidget *parent, const QString &name, const QString &placeholder = {}) {
    auto *e = new QLineEdit(parent);
    e->setObjectName(name);
    e->setAccessibleName(placeholder);
    e->setPlaceholderText(placeholder);
    return e;
}
QSpinBox *number(QWidget *parent, const QString &name, int min, int max, int value) {
    auto *s = new QSpinBox(parent);
    s->setObjectName(name);
    s->setRange(min, max);
    s->setValue(value);
    return s;
}
void moneyChips(QVBoxLayout *layout, MoneyLineEdit *edit, QWidget *parent) {
    auto *row = new QHBoxLayout;
    for (int v : {1, 5, 10, 50}) {
        auto *chip = new Chip(QString::number(v) + " triệu", parent);
        row->addWidget(chip);
        QObject::connect(chip, &QPushButton::clicked, edit,
                         [edit, v] { edit->setValue(static_cast<qint64>(v) * 1000000); });
    }
    layout->addLayout(row);
}
QHBoxLayout *dialogActions(QDialog *dialog, QPushButton *primary) {
    auto *row = new QHBoxLayout;
    row->addStretch();
    auto *cancel = button("Đóng", "dialogClose", dialog, "ghost");
    row->addWidget(cancel);
    row->addWidget(primary);
    primary->setDefault(true);
    QObject::connect(cancel, &QPushButton::clicked, dialog, &QDialog::reject);
    return row;
}
QFormLayout *form(QWidget *page) {
    auto *f = new QFormLayout(page);
    f->setContentsMargins(0, 8, 0, 8);
    f->setSpacing(12);
    f->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    return f;
}
struct Draft {
    qint64 start{1000000}, increment{100000}, deposit{100000};
    int delay{5}, duration{60};
};
Draft lastDraft;
QMap<QString, QVariant> lastProductFields;
int lastProductType = 0;
QString lastImage;
} // namespace
TopUpDialog::TopUpDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("topUpDialog");
    setWindowTitle("Nạp tiền vào ví");
    resize(500, 300);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    layout->addWidget(label("Nạp tiền vào ví", this, 28));
    layout->addWidget(label("Số tiền sẽ được cộng vào số dư khả dụng của người đang thao tác.",
                            this, 14, "muted"));
    amount_ = new MoneyLineEdit(this);
    amount_->setObjectName("topUpAmount");
    layout->addWidget(amount_);
    moneyChips(layout, amount_, this);
    auto *ok = button("Nạp tiền", "topUpSubmit", this, "primary");
    layout->addLayout(dialogActions(this, ok));
    connect(amount_, &MoneyLineEdit::valueChanged, this, [ok](qint64 v) { ok->setEnabled(v > 0); });
    ok->setEnabled(false);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    amount_->setFocus();
}
ConfirmJoinDialog::ConfirmJoinDialog(const application::AuctionDetailDto &a,
                                     const application::WalletDto &wallet, QWidget *parent)
    : QDialog(parent) {
    setObjectName("confirmJoinDialog");
    setWindowTitle("Xác nhận ký quỹ");
    resize(540, 380);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    layout->addWidget(label("Tham gia đấu giá", this, 28));
    layout->addWidget(label(QString::fromStdString(a.productName), this, 18));
    layout->addWidget(
        label("Cọc cố định: " + Formatters::formatMoney(a.depositAmount), this, 20, "accent"));
    layout->addWidget(label("Khoản cọc không phụ thuộc giá bạn đặt. Cọc được hoàn nếu bạn thua "
                            "hoặc phiên bị hủy; được trừ vào giá thanh toán nếu bạn thắng.",
                            this, 14, "muted"));
    const bool enough = wallet.available >= a.depositAmount;
    layout->addWidget(
        label("Khả dụng trước: " + Formatters::formatMoney(wallet.available) + "\nKhả dụng sau: " +
                  (enough ? Formatters::formatMoney(wallet.available - a.depositAmount)
                          : "Không đủ số dư, hãy nạp tiền"),
              this, 16, enough ? "" : "danger"));
    auto *ok = button("Xác nhận đặt cọc", "confirmJoinSubmit", this, "primary");
    ok->setEnabled(enough);
    layout->addLayout(dialogActions(this, ok));
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
}
ConfirmCloseEarlyDialog::ConfirmCloseEarlyDialog(const application::AuctionDetailDto &a,
                                                 bool canPay, QWidget *parent)
    : QDialog(parent) {
    setObjectName("confirmCloseEarlyDialog");
    setWindowTitle("Chốt phiên sớm");
    resize(580, 440);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    layout->addWidget(label("Chốt phiên sớm", this, 28));
    layout->addWidget(label(QString::fromStdString(a.productName), this, 18));
    layout->addWidget(label("Người thắng: " + QString::fromStdString(a.leaderName) +
        "\nGiá chốt: " + Formatters::formatMoney(a.currentPrice) +
        "\nCọc trừ vào giá: " + Formatters::formatMoney(a.depositAmount) +
        "\nNgười thắng trả thêm: " + Formatters::formatMoney(a.currentPrice - a.depositAmount) +
        "\nSố người được hoàn cọc: " + QString::number(a.participants.size() - 1),
        this, 16));
    layout->addWidget(label(canPay ?
        "Phiên sẽ đóng ngay và quyết toán. Thao tác này không thể hoàn tác." :
        "Người thắng hiện không đủ tiền thanh toán; cọc sẽ bị tịch thu theo quy định. "
        "Phiên sẽ đóng ngay. Thao tác này không thể hoàn tác.", this, 14, "warning"));
    auto *confirm = button("Xác nhận chốt phiên", "confirmCloseEarlySubmit", this, "danger");
    layout->addLayout(dialogActions(this, confirm));
    connect(confirm, &QPushButton::clicked, this, &QDialog::accept);
}
ConfirmCancelDialog::ConfirmCancelDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("confirmCancelDialog");
    setWindowTitle("Hủy phiên đấu giá");
    resize(520, 300);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    layout->addWidget(label("Hủy phiên đấu giá", this, 28));
    layout->addWidget(label("Phiên sẽ dừng ngay. Hệ thống hoàn tiền cọc cho mọi người tham gia. "
                            "Thao tác này không thể hoàn tác.",
                            this, 14, "warning"));
    reason_ = input(this, "cancelReason", "Lý do hủy phiên");
    layout->addWidget(reason_);
    auto *ok = button("Xác nhận hủy phiên", "confirmCancelSubmit", this, "danger");
    ok->setEnabled(false);
    layout->addLayout(dialogActions(this, ok));
    connect(reason_, &QLineEdit::textChanged, this,
            [this, ok] { ok->setEnabled(!reason_->text().trimmed().isEmpty()); });
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    reason_->setFocus();
}
CreateAuctionWizard::CreateAuctionWizard(UiContext &c, QWidget *parent) : QDialog(parent), c_(c) {
    setObjectName("createAuctionWizard");
    setWindowTitle("Tạo phiên đấu giá");
    resize(760, 720);
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(24, 24, 24, 24);
    main->setSpacing(16);
    main->addWidget(label("Tạo phiên đấu giá", this, 28));
    indicator_ = new StepIndicator({"Sản phẩm", "Giá & ký quỹ", "Thời gian", "Xem lại"}, this);
    main->addWidget(indicator_);
    pages_ = new QStackedWidget(this);
    main->addWidget(pages_, 1);
    auto *product = new QWidget(this);
    auto *productLayout = new QVBoxLayout(product);
    productLayout->setContentsMargins(0, 0, 0, 0);
    productLayout->setSpacing(12);
    auto *categories = new QHBoxLayout;
    auto *group = new QButtonGroup(this);
    const QStringList kinds = {"Đồ cổ", "Tranh nghệ thuật", "Thiết bị công nghệ"};
    const QStringList icons = {"vase", "frame", "cpu"};
    productFields_ = new QStackedWidget(product);
    productFields_->setObjectName("productTypeFields");
    for (int i = 0; i < 3; ++i) {
        auto *choice = new Chip(kinds[i], product);
        choice->setObjectName("productType" + QString::number(i));
        choice->setIcon(IconProvider::icon(icons[i], QColor(c.theme.theme().accent), 24));
        choice->setMinimumHeight(42);
        group->addButton(choice, i);
        categories->addWidget(choice, 1);
        if (i == 0)
            choice->setChecked(true);
        connect(choice, &QPushButton::clicked, this, [this, i] {
            productFields_->setCurrentIndex(i);
            validate();
        });
    }
    productLayout->addLayout(categories);
    name_ = input(product, "productName", "Tên sản phẩm");
    description_ = input(product, "productDescription", "Mô tả ngắn");
    productLayout->addWidget(name_);
    productLayout->addWidget(description_);
    auto *antique = new QWidget(productFields_);
    auto *af = form(antique);
    century_ = number(antique, "productCentury", 1, 30, 19);
    origin_ = input(antique, "productOrigin", "Xuất xứ");
    preservation_ = input(antique, "productPreservation", "Tình trạng bảo quản");
    appraised_ = new QCheckBox("Đã giám định", antique);
    af->addRow("Thế kỷ", century_);
    af->addRow("Xuất xứ", origin_);
    af->addRow("Tình trạng", preservation_);
    af->addRow(appraised_);
    productFields_->addWidget(antique);
    auto *art = new QWidget(productFields_);
    auto *artf = form(art);
    artist_ = input(art, "productArtist", "Họa sĩ");
    year_ = number(art, "productYear", 1, QDate::currentDate().year(), QDate::currentDate().year());
    medium_ = input(art, "productMedium", "Chất liệu");
    dimensions_ = input(art, "productDimensions", "Kích thước");
    certificate_ = new QCheckBox("Có chứng nhận xác thực", art);
    artf->addRow("Họa sĩ", artist_);
    artf->addRow("Năm sáng tác", year_);
    artf->addRow("Chất liệu", medium_);
    artf->addRow("Kích thước", dimensions_);
    artf->addRow(certificate_);
    productFields_->addWidget(art);
    auto *tech = new QWidget(productFields_);
    auto *tf = form(tech);
    brand_ = input(tech, "productBrand", "Hãng");
    model_ = input(tech, "productModel", "Model");
    warranty_ = number(tech, "productWarranty", 0, 120, 12);
    isNew_ = new QCheckBox("Sản phẩm mới", tech);
    tf->addRow("Hãng", brand_);
    tf->addRow("Model", model_);
    tf->addRow("Bảo hành (tháng)", warranty_);
    tf->addRow(isNew_);
    productFields_->addWidget(tech);
    productLayout->addWidget(productFields_);
    auto *image = button("Chọn ảnh sản phẩm", "chooseProductImage", product, "ghost");
    auto *clearImage = button("Bỏ ảnh", "clearProductImage", product, "ghost");
    clearImage->hide();
    auto *imageRow = new QHBoxLayout;
    imageRow->addWidget(image, 1);imageRow->addWidget(clearImage);
    productLayout->addLayout(imageRow);
    connect(clearImage, &QPushButton::clicked, this, [this, image, clearImage] {
        imagePath_.clear();image->setText("Chọn ảnh sản phẩm");image->setToolTip({});clearImage->hide();
    });
    connect(image, &QPushButton::clicked, this, [this, image, clearImage] {
        const auto path = QFileDialog::getOpenFileName(this, "Chọn ảnh sản phẩm", {},
                                                       "Ảnh (*.png *.jpg *.jpeg *.webp *.bmp)");
        if (!path.isEmpty()) {
            imagePath_ = path;
            image->setText("Đã chọn ảnh · Thay ảnh");
            image->setToolTip(path);
            clearImage->show();
        }
    });
    productLayout->addStretch();
    pages_->addWidget(scrollPage(product, pages_));
    auto *price = new QWidget(this);
    auto *pl = new QVBoxLayout(price);
    pl->setSpacing(16);
    starting_ = new MoneyLineEdit(price);
    starting_->setObjectName("startingPrice");
    increment_ = new MoneyLineEdit(price);
    increment_->setObjectName("minIncrement");
    deposit_ = new MoneyLineEdit(price);
    deposit_->setObjectName("depositAmount");
    starting_->setValue(lastDraft.start);
    increment_->setValue(lastDraft.increment);
    deposit_->setValue(lastDraft.deposit);
    for (const auto &pair :
         QList<QPair<QString, MoneyLineEdit *>>{{"Giá khởi điểm", starting_},
                                                {"Bước giá tối thiểu", increment_},
                                                {"Mức ký quỹ (đặt cọc)", deposit_}}) {
        pl->addWidget(label(pair.first, price, 16));
        pl->addWidget(pair.second);
    }
    pl->addWidget(label("Người tham gia phải đặt cọc khoản này để được đặt giá. Khoản cọc cố định, "
                        "không phụ thuộc giá họ đặt. Gợi ý mặc định: " +
                            QString::number(domain::kDefaultDepositPercent) + "% giá khởi điểm.",
                        price, 14, "muted"));
    pl->addStretch();
    pages_->addWidget(price);
    auto *time = new QWidget(this);
    auto *tl = new QVBoxLayout(time);
    tl->setSpacing(16);
    delay_ = number(time, "startDelay", 1, 86400, lastDraft.delay);
    duration_ = number(time, "duration", 1, 86400, lastDraft.duration);
    delayUnit_ = new QComboBox(time);
    durationUnit_ = new QComboBox(time);
    for (auto *combo : {delayUnit_, durationUnit_}) {
        combo->addItem("Giây", 1);
        combo->addItem("Phút", 60);
        combo->addItem("Giờ", 3600);
    }
    delayUnit_->setObjectName("startDelayUnit");
    durationUnit_->setObjectName("durationUnit");
    tl->addWidget(label("Bắt đầu sau (tính từ lúc bạn duyệt phiên)", time, 20));
    auto *dr = new QHBoxLayout;
    dr->addWidget(delay_, 1);
    dr->addWidget(delayUnit_);
    tl->addLayout(dr);
    auto *dp = new QHBoxLayout;
    for (int n : {5, 60, 300}) {
        auto *b = new Chip(n == 5 ? "5 giây" : QString::number(n / 60) + " phút", time);
        dp->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, n] {
            delayUnit_->setCurrentIndex(0);
            delay_->setValue(n);
        });
    }
    tl->addLayout(dp);
    tl->addWidget(label("Kéo dài", time, 20));
    auto *du = new QHBoxLayout;
    du->addWidget(duration_, 1);
    du->addWidget(durationUnit_);
    tl->addLayout(du);
    auto *presets = new QHBoxLayout;
    for (int n : {60, 300, 900, 3600}) {
        auto *b = new Chip(n == 3600 ? "1 giờ" : QString::number(n / 60) + " phút", time);
        presets->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, n] {
            durationUnit_->setCurrentIndex(0);
            duration_->setValue(n);
        });
    }
    tl->addLayout(presets);
    times_ = label("", time, 16, "info");
    tl->addWidget(times_);
    tl->addStretch();
    pages_->addWidget(time);
    auto *reviewPage = new QWidget(this);
    auto *rl = new QVBoxLayout(reviewPage);
    auto *reviewCard = new Card(reviewPage);
    review_ = label("", reviewCard, 16);
    reviewCard->content->addWidget(review_);
    rl->addWidget(reviewCard);
    rl->addStretch();
    pages_->addWidget(reviewPage);
    error_ = label("", this, 12, "danger");
    error_->setObjectName("wizardError");
    main->addWidget(error_);
    auto *bottom = new QHBoxLayout;
    back_ = button("Quay lại", "wizardBack", this, "ghost");
    next_ = button("Tiếp tục  →", "wizardNext", this, "primary");
    create_ = button("Tạo phiên", "wizardCreate", this, "primary");
    auto *close = button("Đóng", "wizardClose", this, "ghost");
    bottom->addWidget(close);
    bottom->addStretch();
    bottom->addWidget(back_);
    bottom->addWidget(next_);
    bottom->addWidget(create_);
    main->addLayout(bottom);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    connect(back_, &QPushButton::clicked, this, [this] { changeStep(step_ - 1); });
    connect(next_, &QPushButton::clicked, this, [this] { changeStep(step_ + 1); });
    connect(create_, &QPushButton::clicked, this, [this] {
        lastDraft = {starting_->value(), increment_->value(), deposit_->value(),
                     delay_->value() * delayUnit_->currentData().toInt(),
                     duration_->value() * durationUnit_->currentData().toInt()};
        emit createRequested();
    });
    for (auto *edit : findChildren<QLineEdit *>())
        connect(edit, &QLineEdit::textChanged, this, &CreateAuctionWizard::validate);
    for (auto *spin : {delay_, duration_})
        connect(spin, &QSpinBox::valueChanged, this, &CreateAuctionWizard::validate);
    for (auto *combo : {delayUnit_, durationUnit_})
        connect(combo, &QComboBox::currentIndexChanged, this, &CreateAuctionWizard::validate);
    connect(starting_, &MoneyLineEdit::valueChanged, this, [this](qint64 value) {
        if (!depositEdited_)
            deposit_->setValue(value / 100 * domain::kDefaultDepositPercent);
        validate();
    });
    connect(deposit_, &QLineEdit::textEdited, this, [this] { depositEdited_ = true; });
    for (auto *edit : findChildren<QLineEdit *>())
        if (edit->objectName().startsWith("product") &&
            lastProductFields.contains(edit->objectName()))
            edit->setText(lastProductFields.value(edit->objectName()).toString());
    for (auto *spin : findChildren<QSpinBox *>())
        if (spin->objectName().startsWith("product") &&
            lastProductFields.contains(spin->objectName()))
            spin->setValue(lastProductFields.value(spin->objectName()).toInt());
    group->button(lastProductType)->setChecked(true);
    productFields_->setCurrentIndex(lastProductType);
    imagePath_ = lastImage;
    appraised_->setChecked(lastProductFields.value("appraised").toBool());
    certificate_->setChecked(lastProductFields.value("certificate").toBool());
    isNew_->setChecked(lastProductFields.value("isNew").toBool());
    if (!imagePath_.isEmpty()) {
        image->setText("Đã chọn ảnh · Thay ảnh");
        image->setToolTip(imagePath_);
        clearImage->show();
    }
    connect(create_, &QPushButton::clicked, this, [this] {
        for (auto *edit : findChildren<QLineEdit *>())
            if (edit->objectName().startsWith("product"))
                lastProductFields[edit->objectName()] = edit->text();
        for (auto *spin : findChildren<QSpinBox *>())
            if (spin->objectName().startsWith("product"))
                lastProductFields[spin->objectName()] = spin->value();
        lastProductFields["appraised"] = appraised_->isChecked();
        lastProductFields["certificate"] = certificate_->isChecked();
        lastProductFields["isNew"] = isNew_->isChecked();
        lastProductType = productFields_->currentIndex();
        lastImage = imagePath_;
    });
    connect(&c.ticker, &UiTicker::tick, this, [this] {
        if (step_ >= 2)
            updateReview();
    });
    connect(&c_.gateway,&client::IAuctionGateway::connectionStateChanged,this,[this](auto){validate();});
    changeStep(0);
    name_->setFocus();
}
void CreateAuctionWizard::changeStep(int step) {
    step_ = qBound(0, step, 3);
    pages_->setCurrentIndex(step_);
    indicator_->setStep(step_);
    back_->setVisible(step_ > 0);
    next_->setVisible(step_ < 3);
    create_->setVisible(step_ == 3);
    next_->setDefault(step_ < 3);
    create_->setDefault(step_ == 3);
    validate();
    updateReview();
}
void CreateAuctionWizard::validate() {
    QString error;
    if (step_ == 0) {
        if (!c_.session.loggedIn())
            error = "Đăng nhập trước khi tạo phiên.";
        else if (name_->text().trimmed().isEmpty())
            error = "Nhập tên sản phẩm để tiếp tục.";
        else if (description_->text().trimmed().isEmpty())
            error = "Thêm mô tả ngắn cho sản phẩm.";
    }
    if (step_ == 1) {
        if (starting_->value() <= 0 || increment_->value() <= 0 || deposit_->value() <= 0)
            error = "Giá khởi điểm, bước giá và tiền cọc phải lớn hơn 0.";
        else if (deposit_->value() > starting_->value())
            error = "Tiền cọc không được lớn hơn giá khởi điểm.";
    }
    if (step_ == 2) {
        if (delay_->value() * delayUnit_->currentData().toInt() < domain::kMinStartLead.count())
            error = "Chọn ít nhất 1 giây kể từ khi duyệt phiên.";
        else if (duration_->value() * durationUnit_->currentData().toInt() <
                 domain::kMinAuctionDuration.count())
            error = "Phiên phải kéo dài ít nhất " +
                    QString::number(domain::kMinAuctionDuration.count()) + " giây.";
    }
    error_->setText(error);
    next_->setEnabled(error.isEmpty());
    create_->setEnabled(error.isEmpty() && c_.gateway.state()==client::ConnectionState::Connected && !c_.store.busy(0));
    create_->setToolTip(c_.gateway.state()==client::ConnectionState::Connected?QString():"Mất kết nối tới máy chủ");
    if (step_ >= 2)
        updateReview();
}
application::CreateAuctionCommand CreateAuctionWizard::command() const {
    application::CreateAuctionCommand cmd;
    cmd.sellerId = c_.session.id();
    auto &p = cmd.productCommand;
    p.sellerId = cmd.sellerId;
    p.type = static_cast<application::ProductType>(productFields_->currentIndex());
    p.name = titleCase(name_->text()).toStdString();
    p.description = description_->text().trimmed().toStdString();
    p.eraCentury = century_->value();
    p.origin = origin_->text().trimmed().toStdString();
    p.preservationState = preservation_->text().trimmed().toStdString();
    p.isAppraised = appraised_->isChecked();
    p.artist = titleCase(artist_->text()).toStdString();
    p.yearCreated = year_->value();
    p.medium = medium_->text().trimmed().toStdString();
    p.dimensions = dimensions_->text().trimmed().toStdString();
    p.hasCertificate = certificate_->isChecked();
    p.brand = brand_->text().trimmed().toStdString();
    p.model = model_->text().trimmed().toStdString();
    p.warrantyMonths = warranty_->value();
    p.isNew = isNew_->isChecked();
    cmd.startingPrice = domain::Money(starting_->value());
    cmd.minIncrement = domain::Money(increment_->value());
    cmd.depositAmount = domain::Money(deposit_->value());
    cmd.startDelay = common::Seconds(delay_->value() * delayUnit_->currentData().toInt());
    cmd.duration = common::Seconds(duration_->value() * durationUnit_->currentData().toInt());
    return cmd;
}
void CreateAuctionWizard::updateReview() {
    const QString times = "Bắt đầu sau " +
        QString::number(delay_->value() * delayUnit_->currentData().toInt()) +
        " giây kể từ khi duyệt · Kéo dài " +
        QString::number(duration_->value() * durationUnit_->currentData().toInt()) +
        " giây\nLịch cụ thể sẽ được tính khi bạn duyệt phiên";
    times_->setText(times);
    if (step_ == 3)
        review_->setText(
            titleCase(name_->text()) + "\n" + description_->text() +
            "\n\nNgười bán: " + QString::fromStdString(c_.session.user->name) +
            "\nGiá khởi điểm: " + Formatters::formatMoney(domain::Money(starting_->value())) +
            "\nBước giá: " + Formatters::formatMoney(domain::Money(increment_->value())) +
            "\nKý quỹ: " + Formatters::formatMoney(domain::Money(deposit_->value())) + "\n\n" +
            times + "\n\n" +
            (imagePath_.isEmpty() ? "Ảnh minh họa theo danh mục" : "Đã chọn ảnh sản phẩm"));
}
void CreateAuctionWizard::showError(const QString &error) {
    error_->setText(error);
}
void CreateAuctionWizard::setImagePath(const QString &path) {
    imagePath_ = path;
    if (auto *image = findChild<QPushButton *>("chooseProductImage")) {
        if (!imagePath_.isEmpty()) {
            image->setText("Đã đính kèm ảnh · Thay ảnh");
            image->setToolTip(imagePath_);
        } else {
            image->setText("Chọn ảnh sản phẩm");
            image->setToolTip({});
        }
    }
    if (auto *clearImage = findChild<QPushButton *>("clearProductImage")) {
        clearImage->setVisible(!imagePath_.isEmpty());
    }
    validate();
    if (step_ >= 2)
        updateReview();
}

void CreateAuctionWizard::prefill(const QString &name, const QString &category, const QString &description, const domain::Money &suggestedPrice) {
    if (name_) name_->setText(name);

    int catIdx = 0;
    const QString lowerCat = category.toLower();
    if (lowerCat.contains("nghệ thuật") || lowerCat.contains("art") || lowerCat.contains("tranh")) {
        catIdx = 1;
    } else if (lowerCat.contains("công nghệ") || lowerCat.contains("tech") || lowerCat.contains("thiết bị")) {
        catIdx = 2;
    }
    auto *chip = findChild<QPushButton *>("productType" + QString::number(catIdx));
    if (chip) chip->setChecked(true);
    if (productFields_) productFields_->setCurrentIndex(catIdx);

    QString cleanDesc = description;
    const int dashIdx = description.lastIndexOf(" - ");
    if (dashIdx != -1) {
        const QString firstPart = description.left(dashIdx).trimmed();
        const QString attrPart = description.mid(dashIdx + 3).trimmed();

        if (firstPart.endsWith(')')) {
            const int openParen = firstPart.lastIndexOf(" (");
            if (openParen != -1) {
                cleanDesc = firstPart.mid(openParen + 2);
                cleanDesc.chop(1);
            }
        }

        if (catIdx == 0) {
            static const QRegularExpression reCentury(QString::fromUtf8(R"(thế kỷ\s+(\d+))"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reOrigin(QString::fromUtf8(R"(Xuất xứ:\s*([^,]+))"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression rePres(QString::fromUtf8(R"(Tình trạng:\s*([^,]+))"), QRegularExpression::CaseInsensitiveOption);
            auto mC = reCentury.match(attrPart);
            if (mC.hasMatch() && century_) century_->setValue(mC.captured(1).toInt());
            auto mO = reOrigin.match(attrPart);
            if (mO.hasMatch() && origin_) origin_->setText(mO.captured(1).trimmed());
            auto mP = rePres.match(attrPart);
            if (mP.hasMatch() && preservation_) preservation_->setText(mP.captured(1).trimmed());
            if (appraised_) appraised_->setChecked(attrPart.contains("Đã giám định", Qt::CaseInsensitive));
        } else if (catIdx == 1) {
            static const QRegularExpression reArtist(QString::fromUtf8(R"(Họa sĩ:\s*([^,]+))"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reYear(QString::fromUtf8(R"(Sáng tác:\s*(\d+))"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reMed(QString::fromUtf8(R"(Chất liệu:\s*([^,]+))"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reDim(QString::fromUtf8(R"(Kích thước:\s*([^,]+))"), QRegularExpression::CaseInsensitiveOption);
            auto mA = reArtist.match(attrPart);
            if (mA.hasMatch() && artist_) artist_->setText(mA.captured(1).trimmed());
            auto mY = reYear.match(attrPart);
            if (mY.hasMatch() && year_) year_->setValue(mY.captured(1).toInt());
            auto mM = reMed.match(attrPart);
            if (mM.hasMatch() && medium_) medium_->setText(mM.captured(1).trimmed());
            auto mD = reDim.match(attrPart);
            if (mD.hasMatch() && dimensions_) dimensions_->setText(mD.captured(1).trimmed());
            if (certificate_) certificate_->setChecked(attrPart.contains("Có chứng thực", Qt::CaseInsensitive));
        } else if (catIdx == 2) {
            static const QRegularExpression reBrand(QString::fromUtf8(R"(Hãng:\s*([^,]+))"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reModel(QString::fromUtf8(R"(Model:\s*([^,]+))"), QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reWarr(QString::fromUtf8(R"(Bảo hành:\s*(\d+))"), QRegularExpression::CaseInsensitiveOption);
            auto mB = reBrand.match(attrPart);
            if (mB.hasMatch() && brand_) brand_->setText(mB.captured(1).trimmed());
            auto mM = reModel.match(attrPart);
            if (mM.hasMatch() && model_) model_->setText(mM.captured(1).trimmed());
            auto mW = reWarr.match(attrPart);
            if (mW.hasMatch() && warranty_) warranty_->setValue(mW.captured(1).toInt());
            if (isNew_) isNew_->setChecked(attrPart.contains("Mới 100%", Qt::CaseInsensitive));
        }
    }

    if (description_) description_->setText(cleanDesc.trimmed());

    if (starting_ && suggestedPrice.amount() > 0) {
        starting_->setValue(suggestedPrice.amount());
        if (increment_) increment_->setValue(std::max<qint64>(100000, suggestedPrice.amount() / 20));
        if (deposit_) deposit_->setValue(std::max<qint64>(50000, suggestedPrice.amount() / 10));
    }

    validate();
}
// ============================================================================
// ChangePasswordDialog
// ============================================================================
ChangePasswordDialog::ChangePasswordDialog(UiContext &context, QWidget *parent)
    : QDialog(parent), c_(context) {
    setObjectName("changePasswordDialog");
    setWindowTitle("Đổi mật khẩu tài khoản");
    resize(460, 400);
    setModal(true);

    const auto &theme = c_.theme.theme();
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(theme.bgSurface, theme.text));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(10);

    auto *title = label("Đổi mật khẩu", this, 20);
    title->setStyleSheet(QString("font-weight: 700; color: %1;").arg(theme.text));
    layout->addWidget(title);

    auto *sub = label("Nhập mật khẩu hiện tại và đặt mật khẩu mới để bảo vệ tài khoản của bạn.", this, 13, "muted");
    sub->setWordWrap(true);
    layout->addWidget(sub);
    layout->addSpacing(4);

    const auto makeField = [this, layout](const QString &caption, const QString &objName) -> QLineEdit * {
        auto *lbl = label(caption, this, 13);
        layout->addWidget(lbl);
        auto *edit = new QLineEdit(this);
        edit->setObjectName(objName);
        edit->setEchoMode(QLineEdit::Password);
        edit->setPlaceholderText(caption);
        edit->setMaxLength(128);
        auto *action = edit->addAction(QIcon(), QLineEdit::TrailingPosition);
        action->setText("Hiện");
        connect(action, &QAction::triggered, edit, [edit, action] {
            const bool hidden = edit->echoMode() == QLineEdit::Password;
            edit->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
            action->setText(hidden ? "Ẩn" : "Hiện");
        });
        layout->addWidget(edit);
        return edit;
    };

    currentPassword_ = makeField("Mật khẩu hiện tại", "edtCurrentPassword");
    newPassword_ = makeField("Mật khẩu mới (8–128 ký tự)", "edtNewPassword");
    repeatPassword_ = makeField("Xác nhận mật khẩu mới", "edtRepeatPassword");

    errorLabel_ = label("", this, 13, "danger");
    errorLabel_->setObjectName("lblChangePasswordError");
    errorLabel_->setWordWrap(true);
    layout->addWidget(errorLabel_);

    layout->addStretch();

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *btnClose = button("Hủy", "btnCancelChangePassword", this, "ghost");
    submitButton_ = button("Lưu mật khẩu mới", "btnSubmitChangePassword", this, "primary");
    submitButton_->setEnabled(false);
    btnRow->addWidget(btnClose);
    btnRow->addWidget(submitButton_);
    layout->addLayout(btnRow);

    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    const auto validate = [this] {
        const QString cur = currentPassword_->text();
        const QString nw = newPassword_->text();
        const QString rep = repeatPassword_->text();

        QString err;
        if (!nw.isEmpty() && (nw.length() < 8 || nw.length() > 128)) {
            err = "Mật khẩu mới phải từ 8 đến 128 ký tự.";
        } else if (!rep.isEmpty() && nw != rep) {
            err = "Mật khẩu xác nhận không khớp.";
        }
        errorLabel_->setText(err);
        submitButton_->setEnabled(!cur.isEmpty() && nw.length() >= 8 && nw.length() <= 128 && nw == rep);
    };

    connect(currentPassword_, &QLineEdit::textChanged, this, validate);
    connect(newPassword_, &QLineEdit::textChanged, this, validate);
    connect(repeatPassword_, &QLineEdit::textChanged, this, validate);

    connect(submitButton_, &QPushButton::clicked, this, [this] {
        errorLabel_->clear();
        submitButton_->setEnabled(false);
        submitButton_->setText("Đang lưu…");
        currentPassword_->setEnabled(false);
        newPassword_->setEnabled(false);
        repeatPassword_->setEnabled(false);

        c_.gateway.changePassword(
            currentPassword_->text().toStdString(),
            newPassword_->text().toStdString(),
            this,
            [this](auto result) {
                submitButton_->setText("Lưu mật khẩu mới");
                currentPassword_->setEnabled(true);
                newPassword_->setEnabled(true);
                repeatPassword_->setEnabled(true);
                if (!result) {
                    errorLabel_->setText(result.error->message);
                    submitButton_->setEnabled(true);
                    return;
                }
                emit passwordChanged();
                accept();
            }
        );
    });
}

} // namespace auction::presentation
