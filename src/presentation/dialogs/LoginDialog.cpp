#include "auction/presentation/dialogs/Dialogs.h"
#include "auction/presentation/util/UiHelpers.h"
#include "auction/presentation/theme/IconProvider.h"
#include "auction/domain/values/PasswordPolicy.h"
#include "auction/domain/values/SecretBuffer.h"
#include <QTabWidget>
#include <QTimer>

namespace auction::presentation {
namespace {
struct FormState {
    bool pending{},submitted{};
    common::SteadyTime retryAt{};
    QString loginErrorMessage,registrationErrorMessage;
};
QLineEdit *field(QVBoxLayout *layout,QWidget *parent,const QString &name,const QString &caption,int max) {
    auto *edit=new QLineEdit(parent);edit->setObjectName(name);edit->setAccessibleName(caption);
    edit->setPlaceholderText(caption);edit->setMaxLength(max);
    auto *captionLabel=label(caption,parent,14);captionLabel->setBuddy(edit);
    layout->addWidget(captionLabel);layout->addWidget(edit);return edit;
}
void passwordToggle(QLineEdit *edit,ThemeManager &theme) {
    edit->setEchoMode(QLineEdit::Password);
    auto *action=edit->addAction(IconProvider::icon("eye",QColor(theme.theme().textMuted)),QLineEdit::TrailingPosition);
    action->setObjectName(edit->objectName()+"Toggle");action->setText("Hiện mật khẩu");action->setToolTip(action->text());
    QObject::connect(action,&QAction::triggered,edit,[edit,action,&theme] {
        const bool hidden=edit->echoMode()==QLineEdit::Password;
        edit->setEchoMode(hidden?QLineEdit::Normal:QLineEdit::Password);
        action->setText(hidden?"Ẩn mật khẩu":"Hiện mật khẩu");action->setToolTip(action->text());
        action->setIcon(IconProvider::icon(hidden?"eye-off":"eye",QColor(theme.theme().textMuted)));
    });
}
QString emailError(const QString &value) {
    try {const auto parsed=domain::Email::parse(value.toStdString());return parsed.value().empty()?QString("Nhập email hợp lệ."):QString();}
    catch(const std::exception &) {return "Nhập email hợp lệ, ví dụ ban@example.com.";}
}
}
LoginDialog::LoginDialog(UiContext &context,QWidget *parent):QDialog(parent),c_(context) {
    setObjectName("loginDialog");setWindowTitle("Tài khoản đấu giá");setMinimumWidth(540);resize(560,650);
    setStyleSheet("QDialog#loginDialog QLineEdit {padding:6px 10px; min-height:18px;} QDialog#loginDialog QLineEdit QToolButton {padding:0; min-height:0; border:none; background:transparent;} QDialog#loginDialog QTabBar::tab {padding:10px 14px;}");
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(24,20,24,20);layout->setSpacing(10);
    layout->addWidget(label("Tài khoản đấu giá",this,26));
    auto *notice=label("",this,14,"warning");notice->setObjectName("sessionNotice");notice->hide();layout->addWidget(notice);
    auto *tabs=new QTabWidget(this);tabs->setObjectName("loginTabs");layout->addWidget(tabs,1);
    auto *login=new QWidget(tabs);auto *ll=new QVBoxLayout(login);ll->setSpacing(8);
    auto *email=field(ll,login,"loginEmail","Email",508);email->setInputMethodHints(Qt::ImhEmailCharactersOnly);
    auto *password=field(ll,login,"loginPassword","Mật khẩu",256);passwordToggle(password,c_.theme);
    auto *forgotLink=button("Quên mật khẩu?","forgotPasswordLink",login,"ghost");
    forgotLink->setCursor(Qt::PointingHandCursor);forgotLink->setStyleSheet("text-align:right; font-size:12px; padding:2px 0; border:none;");
    ll->addWidget(forgotLink,0,Qt::AlignRight);
    auto *loginError=label("",login,14,"danger");loginError->setObjectName("loginError");loginError->setMinimumHeight(44);ll->addWidget(loginError);
    ll->addStretch();auto *submit=button("Đăng nhập","loginSubmit",login,"primary");ll->addWidget(submit);
    auto *registration=new QWidget(tabs);auto *rl=new QVBoxLayout(registration);rl->setSpacing(5);
    auto makeError=[&](const QString &name) {auto *error=label("",registration,13,"danger");error->setObjectName(name);error->setMinimumHeight(18);rl->addWidget(error);return error;};
    auto *name=field(rl,registration,"registerName","Tên hiển thị",128);auto *nameError=makeError("registerNameError");
    auto *registerEmail=field(rl,registration,"registerEmail","Email",508);registerEmail->setInputMethodHints(Qt::ImhEmailCharactersOnly);auto *mailError=makeError("registerEmailError");
    auto *secret=field(rl,registration,"registerPassword","Mật khẩu (8–128 ký tự)",256);passwordToggle(secret,c_.theme);auto *secretError=makeError("registerPasswordError");
    auto *strength=label("Dùng mật khẩu dài, riêng cho tài khoản này.",registration,13,"muted");strength->setObjectName("passwordStrength");rl->addWidget(strength);
    auto *repeat=field(rl,registration,"registerRepeat","Nhập lại mật khẩu",256);passwordToggle(repeat,c_.theme);auto *repeatError=makeError("registerRepeatError");
    auto *registerError=label("",registration,14,"danger");registerError->setObjectName("registerError");registerError->setMinimumHeight(20);rl->addWidget(registerError);
    auto *existing=button("Chuyển sang Đăng nhập","emailTakenLogin",registration,"ghost");existing->hide();rl->addWidget(existing);
    rl->addStretch();auto *create=button("Đăng ký","registerSubmit",registration,"primary");rl->addWidget(create);
    auto *forgot=new QWidget(tabs);auto *fl=new QVBoxLayout(forgot);fl->setSpacing(6);
    fl->addWidget(label("Nhập email đã đăng ký để cấp lại và đặt mật khẩu mới.",forgot,13,"muted"));
    auto *forgotEmail=field(fl,forgot,"forgotEmail","Email đã đăng ký",508);forgotEmail->setInputMethodHints(Qt::ImhEmailCharactersOnly);
    auto *forgotMailError=label("",forgot,13,"danger");forgotMailError->setObjectName("forgotEmailError");forgotMailError->setMinimumHeight(18);fl->addWidget(forgotMailError);
    auto *forgotNewPassword=field(fl,forgot,"forgotNewPassword","Mật khẩu mới (8–128 ký tự)",256);passwordToggle(forgotNewPassword,c_.theme);
    auto *forgotPassError=label("",forgot,13,"danger");forgotPassError->setObjectName("forgotPassError");forgotPassError->setMinimumHeight(18);fl->addWidget(forgotPassError);
    auto *forgotRepeat=field(fl,forgot,"forgotRepeat","Nhập lại mật khẩu mới",256);passwordToggle(forgotRepeat,c_.theme);
    auto *forgotRepeatError=label("",forgot,13,"danger");forgotRepeatError->setObjectName("forgotRepeatError");forgotRepeatError->setMinimumHeight(18);fl->addWidget(forgotRepeatError);
    auto *forgotError=label("",forgot,14,"danger");forgotError->setObjectName("forgotError");forgotError->setMinimumHeight(24);fl->addWidget(forgotError);
    fl->addStretch();
    auto *resetSubmit=button("Cấp lại & Đặt mật khẩu mới","resetSubmit",forgot,"primary");fl->addWidget(resetSubmit);
    auto *backToLogin=button("Quay lại Đăng nhập","backToLogin",forgot,"ghost");fl->addWidget(backToLogin);
    tabs->addTab(login,"Đăng nhập");tabs->addTab(registration,"Đăng ký");tabs->addTab(forgot,"Quên mật khẩu");
    auto *close=button("Đóng","dialogClose",this,"ghost");layout->addWidget(close,0,Qt::AlignRight);
    connect(close,&QPushButton::clicked,this,&QDialog::reject);
    connect(existing,&QPushButton::clicked,this,[=]{email->setText(registerEmail->text());tabs->setCurrentIndex(0);password->setFocus();});
    connect(forgotLink,&QPushButton::clicked,this,[=]{forgotEmail->setText(email->text());tabs->setCurrentIndex(2);forgotNewPassword->setFocus();});
    connect(backToLogin,&QPushButton::clicked,this,[=]{tabs->setCurrentIndex(0);email->setFocus();});
    auto state=std::make_shared<FormState>();
    const auto update=[=,this] {
        const auto remaining=std::max<qint64>(0,std::chrono::duration_cast<common::Millis>(state->retryAt-c_.gateway.clock().now()).count());
        const bool ready=!state->pending && remaining==0 && c_.gateway.state()==client::ConnectionState::Connected;
        submit->setEnabled(ready && !email->text().trimmed().isEmpty() && !password->text().isEmpty());
        submit->setText(state->pending?"Đang đăng nhập…":"Đăng nhập");
        create->setText(state->pending?"Đang đăng ký…":"Đăng ký");
        const auto connection=c_.gateway.state();
        const QString connectionHint=connection==client::ConnectionState::Connected?QString():
            connection==client::ConnectionState::Disconnected?"Chưa kết nối máy chủ. Hãy chạy AuctionServer trước.":"Đang kết nối máy chủ, vui lòng chờ…";
        const QString retryHint=remaining>0?"Thử lại sau "+QString::number((remaining+999)/1000)+" giây":QString();
        loginError->setText(!connectionHint.isEmpty()?connectionHint:!retryHint.isEmpty()?retryHint:state->loginErrorMessage);
        tone(loginError,!connectionHint.isEmpty() || !retryHint.isEmpty()?"warning":"danger");
        const bool show=state->submitted;
        const auto display=[show](QLabel *error,const QString &value,bool entered){error->setText(show || entered?value:QString());};
        const auto n=name->text().trimmed();const QString nError=n.isEmpty() || n.toUcs4().size()>64?"Tên hiển thị cần 1–64 ký tự.":QString();
        const auto eError=emailError(registerEmail->text());QString pError;
        auto plain=secret->text().toStdString();domain::SecretBuffer wipe(plain);
        try {domain::PasswordPolicy::validate(plain,domain::Email::parse(eError.isEmpty()?registerEmail->text().toStdString():"policy@validation.local"),n.toStdString());}
        catch(const domain::AccountValidationException &e) {if(e.reason()=="WeakPassword")pError="Dùng 8–128 ký tự; tránh tên, email và mật khẩu phổ biến.";}
        const QString mismatch=secret->text()!=repeat->text() || repeat->text().isEmpty()?"Mật khẩu nhập lại chưa khớp.":QString();
        display(nameError,nError,!name->text().isEmpty());display(mailError,eError,!registerEmail->text().isEmpty());
        display(secretError,pError,!secret->text().isEmpty());display(repeatError,mismatch,!repeat->text().isEmpty());
        const auto length=secret->text().toUcs4().size();strength->setText(length==0?"Dùng mật khẩu dài, riêng cho tài khoản này.":length<8?"Chưa đủ dài · cần ít nhất 8 ký tự.":length<12?"Đủ độ dài · thêm vài từ để dễ nhớ và khó đoán hơn.":"Độ dài tốt · tránh dùng lại mật khẩu ở nơi khác.");
        create->setEnabled(ready && nError.isEmpty() && eError.isEmpty() && pError.isEmpty() && mismatch.isEmpty() && length>=8);
        QString hint;
        QString hintTone="muted";
        if(!connectionHint.isEmpty()) {hint=connectionHint;hintTone="warning";}
        else if(!retryHint.isEmpty()) {hint=retryHint;hintTone="warning";}
        else if(state->pending)hint="Đang xử lý tài khoản, vui lòng chờ…";
        else if(!state->registrationErrorMessage.isEmpty()) {hint=state->registrationErrorMessage;hintTone="danger";}
        else if(n.isEmpty())hint="Nhập tên hiển thị để tiếp tục.";
        else if(!nError.isEmpty())hint=nError;
        else if(registerEmail->text().trimmed().isEmpty())hint="Nhập email để tiếp tục.";
        else if(!eError.isEmpty())hint=eError;
        else if(secret->text().isEmpty())hint="Nhập mật khẩu có 8–128 ký tự.";
        else if(!pError.isEmpty())hint=pError;
        else if(repeat->text().isEmpty())hint="Nhập lại mật khẩu để xác nhận.";
        else if(!mismatch.isEmpty())hint=mismatch;
        registerError->setText(hint);tone(registerError,hintTone);
        create->setToolTip(create->isEnabled()?QString():hint);
        const auto fMailError=emailError(forgotEmail->text());
        const bool fPassLen=forgotNewPassword->text().length()>=8 && forgotNewPassword->text().length()<=128;
        const QString fMismatch=forgotNewPassword->text()!=forgotRepeat->text() || forgotRepeat->text().isEmpty()?"Mật khẩu mới nhập lại chưa khớp.":QString();
        display(forgotMailError,fMailError,!forgotEmail->text().isEmpty());
        display(forgotPassError,!fPassLen && !forgotNewPassword->text().isEmpty()?"Mật khẩu mới cần từ 8–128 ký tự.":QString(),!forgotNewPassword->text().isEmpty());
        display(forgotRepeatError,fMismatch,!forgotRepeat->text().isEmpty());
        resetSubmit->setEnabled(ready && fMailError.isEmpty() && fPassLen && fMismatch.isEmpty());
        resetSubmit->setText(state->pending?"Đang xử lý…":"Cấp lại & Đặt mật khẩu mới");
        for(auto *edit:{email,password,name,registerEmail,secret,repeat,forgotEmail,forgotNewPassword,forgotRepeat})edit->setEnabled(!state->pending);
    };
    const auto fail=[=,this](const client::GatewayError &error,QLabel *target) {
        state->pending=false;
        if(error.type=="RateLimited")state->retryAt=c_.gateway.clock().now()+common::Millis(qMax<qint64>(1,error.data.value("retryAfterMs").toVariant().toLongLong()));
        const auto message=error.type=="RateLimited"?QString():error.type=="Authentication"?"Email hoặc mật khẩu không đúng":error.reason=="EmailTaken"?"Email đã được đăng ký":error.message;
        (target==registerError?state->registrationErrorMessage:state->loginErrorMessage)=message;
        existing->setVisible(error.reason=="EmailTaken");
        update();
    };
    auto authenticate=[=,this] {
        if(!submit->isEnabled())return;
        auto plain=password->text().toStdString();domain::SecretBuffer wipe(plain);
        state->pending=true;state->loginErrorMessage.clear();password->clear();update();
        c_.gateway.authenticate(email->text().toStdString(),plain,this,[=,this](auto result) {
            if(!result){fail(*result.error,loginError);password->setFocus();return;}
            state->pending=false;user_=std::move(*result.value);accept();
        });
    };
    connect(submit,&QPushButton::clicked,this,authenticate);connect(password,&QLineEdit::returnPressed,this,authenticate);
    connect(email,&QLineEdit::returnPressed,password,[password]{password->setFocus();});
    connect(create,&QPushButton::clicked,this,[=,this] {
        state->submitted=true;update();if(!create->isEnabled())return;
        application::RegisterUserCommand command{name->text().toStdString(),secret->text().toStdString(),registerEmail->text().toStdString()};domain::SecretBuffer wipe(command.password);
        state->pending=true;state->registrationErrorMessage.clear();existing->hide();update();
        c_.gateway.registerUser(std::move(command),this,[=,this](auto result) {
            if(!result){fail(*result.error,registerError);return;}
            state->pending=false;user_=std::move(*result.value);accept();
        });
    });
    connect(repeat,&QLineEdit::returnPressed,create,[create]{if(create->isEnabled())create->click();});
    connect(resetSubmit,&QPushButton::clicked,this,[=,this] {
        if(!resetSubmit->isEnabled())return;
        state->pending=true;forgotError->clear();update();
        c_.gateway.resetPassword(forgotEmail->text().trimmed().toStdString(),forgotNewPassword->text().toStdString(),this,[=,this](auto result) {
            state->pending=false;
            if(!result){forgotError->setText(result.error->message);update();return;}
            user_=std::move(*result.value);accept();
        });
    });
    connect(forgotRepeat,&QLineEdit::returnPressed,resetSubmit,[resetSubmit]{if(resetSubmit->isEnabled())resetSubmit->click();});
    for(auto *edit:{email,password,forgotEmail,forgotNewPassword,forgotRepeat})connect(edit,&QLineEdit::textChanged,this,update);
    for(auto *edit:{name,registerEmail,secret,repeat})connect(edit,&QLineEdit::textChanged,this,[state,existing,update] {
        state->registrationErrorMessage.clear();existing->hide();update();
    });
    auto *countdown=new QTimer(this);countdown->setInterval(100);connect(countdown,&QTimer::timeout,this,update);countdown->start();
    connect(&c_.gateway,&client::IAuctionGateway::connectionStateChanged,this,update);
    connect(this,&QDialog::finished,this,[=]{for(auto *edit:{password,secret,repeat,forgotNewPassword,forgotRepeat}){edit->setText(QString(edit->text().size(),QChar(0)));edit->clear();}});
    connect(tabs,&QTabWidget::currentChanged,this,[=](int index){if(index==0)email->setFocus();else if(index==1)name->setFocus();else forgotEmail->setFocus();});
    // Return is handled by the focused field. A disabled submit must not redirect
    // the same key to another dialog default action and close an in-flight login.
    for(auto *action:findChildren<QPushButton *>()){action->setAutoDefault(false);action->setDefault(false);}
    QWidget::setTabOrder(email,password);QWidget::setTabOrder(password,submit);QWidget::setTabOrder(name,registerEmail);QWidget::setTabOrder(registerEmail,secret);QWidget::setTabOrder(secret,repeat);QWidget::setTabOrder(repeat,create);
    update();email->setFocus();
}
void LoginDialog::setNotice(const QString &message) {auto *notice=findChild<QLabel *>("sessionNotice");notice->setText(message);notice->setVisible(!message.isEmpty());}
LoginDialog::~LoginDialog() {for(auto *edit:findChildren<QLineEdit *>())if(edit->objectName().contains("Password") || edit->objectName()=="registerRepeat"){edit->setText(QString(edit->text().size(),QChar(0)));edit->clear();}}
}
