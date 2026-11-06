#pragma once
#include "auction/domain/AuctionConfig.h"
#include "auction/domain/values/Email.h"
#include <array>
#include <algorithm>
namespace auction::domain {
class PasswordPolicy final {
public:
    static void validate(std::string_view plain,const Email &email,std::string_view name) {
        std::u32string text,folded;std::string normalized;
        struct Wipe {
            std::u32string &a,&b;std::string &c;
            ~Wipe() {
                for(auto *s:{&a,&b}) {volatile char32_t *bytes=s->data();for(size_t i=0;i<s->size();++i)bytes[i]=0;s->clear();}
                volatile char *bytes=c.data();for(size_t i=0;i<c.size();++i)bytes[i]=0;c.clear();
            }
        } wipe{text,folded,normalized};
        try {text=account_text::decode(plain);}catch(const std::invalid_argument &){weak();}
        if(text.size()<kMinPasswordLength || text.size()>kMaxPasswordLength ||
           std::all_of(text.begin(),text.end(),account_text::space))weak();
        folded=account_text::fold(text);
        if(folded==account_text::fold(account_text::decode(email.value())) ||
           folded==account_text::fold(account_text::trim(account_text::decode(name))))weak();
        static constexpr std::array<std::string_view,50> common{
            "12345678","123456789","1234567890","12345678910","12345678901",
            "password","password1","password123","qwerty123","qwertyuiop",
            "qwerty12","11111111","00000000","123123123","12341234",
            "87654321","abcdefgh","abc12345","abc123456","abcd1234",
            "iloveyou","letmein1","welcome1","welcome123","admin123",
            "administrator","passw0rd","passw0rd1","football","baseball",
            "sunshine","princess","computer","superman","trustno1",
            "whatever","dragon123","monkey123","master123","internet",
            "changeme","changeme123","test1234","test12345","aaaaaaaa",
            "99999999","88888888","22222222","123456aa","password!"};
        normalized=account_text::encode(folded);
        if(std::find(common.begin(),common.end(),normalized)!=common.end())weak();
    }
private:
    [[noreturn]] static void weak() {throw AccountValidationException("WeakPassword","Mật khẩu phải có 8–128 ký tự, không trùng tên/email và không quá phổ biến");}
};
}
