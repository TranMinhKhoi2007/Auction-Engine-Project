#pragma once
#include "auction/presentation/app/UiContext.h"
#include "auction/presentation/widgets/Components.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QStackedWidget>
namespace auction::presentation {
class ServerSettingsDialog final : public QDialog {
public:
    explicit ServerSettingsDialog(QUrl url,QWidget *parent=nullptr);
    QUrl url() const {return QUrl(edit_->text().trimmed());}
    QString caCertificateFile() const;
private:
    QLineEdit *edit_{};
    QLineEdit *caFile_{};
};
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(UiContext &context, QWidget *parent = nullptr);
    ~LoginDialog() override;
    void setNotice(const QString &message);
    std::optional<application::AccountDto> authenticatedUser() const { return user_; }

private:
    UiContext &c_;
    std::optional<application::AccountDto> user_;
};
class TopUpDialog : public QDialog {
public:
    explicit TopUpDialog(QWidget *parent = nullptr);
    domain::Money amount() const { return domain::Money(amount_->value()); }

private:
    MoneyLineEdit *amount_;
};
class ConfirmJoinDialog : public QDialog {
public:
    ConfirmJoinDialog(const application::AuctionDetailDto &auction,
                      const application::WalletDto &wallet, QWidget *parent = nullptr);
};
class ConfirmCloseEarlyDialog : public QDialog {
public:
    ConfirmCloseEarlyDialog(const application::AuctionDetailDto &auction,
                            bool canPay, QWidget *parent = nullptr);
};
class ConfirmCancelDialog : public QDialog {
public:
    explicit ConfirmCancelDialog(QWidget *parent = nullptr);
    QString reason() const { return reason_->text().trimmed(); }

private:
    QLineEdit *reason_;
};
class CreateAuctionWizard : public QDialog {
    Q_OBJECT
public:
    CreateAuctionWizard(UiContext &context, QWidget *parent = nullptr);
    void prefill(const QString &name, const QString &category, const QString &description, const domain::Money &suggestedPrice);
    application::CreateAuctionCommand command() const;
    QString imagePath() const { return imagePath_; }
    void setImagePath(const QString &path);
    void showError(const QString &message);
signals:
    void createRequested();

private:
    void validate();
    void updateReview();
    void changeStep(int step);
    UiContext &c_;
    int step_{};
    QString imagePath_;
    QStackedWidget *pages_, *productFields_;
    StepIndicator *indicator_;
    QPushButton *next_, *back_, *create_;
    QLabel *error_, *review_, *times_;
    QComboBox *delayUnit_, *durationUnit_;
    QLineEdit *name_, *description_, *origin_, *preservation_, *artist_, *medium_, *dimensions_,
        *brand_, *model_;
    QSpinBox *century_, *year_, *warranty_, *delay_, *duration_;
    QCheckBox *appraised_, *certificate_, *isNew_;
    MoneyLineEdit *starting_, *increment_, *deposit_;
    bool depositEdited_{false};
};
class ChangePasswordDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChangePasswordDialog(UiContext &context, QWidget *parent = nullptr);
signals:
    void passwordChanged();
private:
    void updateState();
    UiContext &c_;
    QLineEdit *currentPassword_{};
    QLineEdit *newPassword_{};
    QLineEdit *repeatPassword_{};
    QLabel *errorLabel_{};
    QPushButton *submitButton_{};
};

} // namespace auction::presentation
