#include "auction/application/AccountService.h"
#include "auction/domain/entities/NguoiDung.h"
#include <cctype>
namespace auction::application {
AccountService::AccountService(domain::IUserRepository &users):users_(users) {
    for(const auto &u:users_.findAll())nextId_=std::max(nextId_,u->id().value()+1);
}
namespace {
// Existing title casing is preserved for account display names.
std::string normalizeName(std::string name) {
    const std::string lower = "àáạảãâầấậẩẫăằắặẳẵèéẹẻẽêềếệểễìíịỉĩòóọỏõôồốộổỗơờớợởỡùúụủũưừứựửữỳýỵỷỹđ";
    const std::string upper = "ÀÁẠẢÃÂẦẤẬẨẪĂẰẮẶẲẴÈÉẸẺẼÊỀẾỆỂỄÌÍỊỈĨÒÓỌỎÕÔỒỐỘỔỖƠỜỚỢỞỠÙÚỤỦŨƯỪỨỰỬỮỲÝỴỶỸĐ";
    bool beginning=true;
    for(size_t i=0;i<name.size();) {
        const auto byte=static_cast<unsigned char>(name[i]);
        if(byte<128) {name[i]=static_cast<char>(beginning?std::toupper(byte):std::tolower(byte));beginning=std::isspace(byte)!=0;++i;}
        else {
            const size_t length=byte<224?2:byte<240?3:4;const auto letter=name.substr(i,length);
            const auto &from=beginning?lower:upper;const auto &to=beginning?upper:lower;
            if(const auto position=from.find(letter);position!=std::string::npos)name.replace(i,length,to.substr(position,length));
            beginning=false;i+=length;
        }
    }
    return name;
}
}
RegistrationDraft AccountService::validateRegistration(std::string_view email,std::string_view name,std::string_view password) const {
    auto parsed=domain::Email::parse(email);std::u32string display;
    try {display=domain::account_text::trim(domain::account_text::decode(name));}
    catch(const std::invalid_argument &){throw domain::AccountValidationException("InvalidName","Tên hiển thị không hợp lệ");}
    if(display.empty() || display.size()>domain::kMaxDisplayNameLength ||
       std::any_of(display.begin(),display.end(),domain::account_text::control))
        throw domain::AccountValidationException("InvalidName","Tên hiển thị cần 1–64 ký tự");
    auto normalized=normalizeName(domain::account_text::encode(display));
    domain::PasswordPolicy::validate(password,parsed,normalized);
    if(users_.emailExists(parsed))throw domain::AccountValidationException("EmailTaken","Email đã được đăng ký");
    return {std::move(parsed),std::move(normalized)};
}
AccountDto AccountService::completeRegistration(const RegistrationDraft &draft,domain::PasswordHash passwordHash) {
    if(users_.emailExists(draft.email))throw domain::AccountValidationException("EmailTaken","Email đã được đăng ký");
    while(users_.findById(common::UserId{nextId_}))++nextId_;
    const common::UserId id{nextId_++};
    auto user=std::make_shared<domain::NguoiDung>(id,draft.name,std::move(passwordHash),draft.email);
    users_.save(user);return accountOf(id);
}
std::optional<CredentialRecord> AccountService::findCredentials(const domain::Email &email) const {
    const auto user=users_.findByEmail(email);
    if(!user)return std::nullopt;
    return CredentialRecord{user->id(),user->passwordHash()};
}
AccountDto AccountService::accountOf(common::UserId id) const {
    const auto user=users_.findById(id);
    if(!user)throw domain::EntityNotFoundException("Người dùng",id.value());
    return {id,user->name(),user->email().value()};
}
void AccountService::updatePasswordHash(common::UserId id,domain::PasswordHash hash) {
    const auto user=users_.findById(id);
    if(!user)throw domain::EntityNotFoundException("Người dùng",id.value());
    user->replacePasswordHash(std::move(hash));
}
}
