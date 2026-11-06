#include "auction/presentation/dialogs/Dialogs.h"
#include "auction/presentation/util/UiHelpers.h"
#include <QSettings>
#include <QFileDialog>
#include "auction/transport/TlsConfiguration.h"
namespace auction::presentation {
ServerSettingsDialog::ServerSettingsDialog(QUrl url,QWidget *parent):QDialog(parent) {
    setObjectName("dlgServerSettings");setWindowTitle("Máy chủ đấu giá");resize(580,360);
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(24,24,24,24);layout->setSpacing(16);
    layout->addWidget(label("Địa chỉ máy chủ",this,20));
    edit_=new QLineEdit(url.toString(),this);edit_->setObjectName("edtServerUrl");edit_->setPlaceholderText("wss://192.168.1.5:8765");layout->addWidget(edit_);
    layout->addWidget(label("Chứng chỉ mạng LAN (.pem/.crt)",this,14));
    auto *certificateRow=new QHBoxLayout;
    caFile_=new QLineEdit(QSettings("AuctionEngine","Client").value("caCertificateFile").toString(),this);caFile_->setObjectName("edtCaCertificate");
    caFile_->setPlaceholderText("Chọn file lan-ca.pem do chủ server cung cấp");certificateRow->addWidget(caFile_,1);
    auto *browse=button("Chọn file…","btnBrowseCa",this,"ghost");certificateRow->addWidget(browse);layout->addLayout(certificateRow);
    auto *hint=label("Chỉ nhận chứng chỉ công khai từ chủ server. Để trống nếu máy đã tin cậy chứng chỉ.",this,12,"muted");hint->setWordWrap(true);layout->addWidget(hint);
    connect(browse,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getOpenFileName(this,"Chọn chứng chỉ LAN",{},"Certificates (*.pem *.crt *.cer)");if(!path.isEmpty())caFile_->setText(path);});
    auto *error=label("",this,12,"danger");error->setObjectName("lblServerUrlError");layout->addWidget(error);
    auto *row=new QHBoxLayout;row->addStretch();auto *close=button("Đóng","btnServerSettingsClose",this,"ghost");
    auto *save=button("Lưu và kết nối","btnSaveServer",this,"primary");row->addWidget(close);row->addWidget(save);layout->addLayout(row);
    const auto validate=[this,error,save] {
        const auto value=this->url();
        bool valid=transport::validServerUrl(value);
        QString message=valid?QString():"Nhập wss://IP:port; ws:// chỉ dùng cho localhost.";
        if(valid && !caFile_->text().trimmed().isEmpty())try {transport::clientTls(caFile_->text().trimmed());}catch(const std::exception &e){valid=false;message=QString::fromUtf8(e.what());}
        error->setText(message);save->setEnabled(valid);
    };
    connect(edit_,&QLineEdit::textChanged,this,validate);connect(close,&QPushButton::clicked,this,&QDialog::reject);
    connect(caFile_,&QLineEdit::textChanged,this,validate);
    connect(save,&QPushButton::clicked,this,[this]{QSettings settings("AuctionEngine","Client");settings.setValue("serverUrl",this->url().toString());settings.setValue("caCertificateFile",caCertificateFile());accept();});validate();
}
QString ServerSettingsDialog::caCertificateFile() const {return caFile_->text().trimmed();}
}
