#include "auction/domain/values/PasswordHash.h"
#include "auction/domain/AuctionConfig.h"
#include <charconv>
namespace auction::domain {
namespace {
constexpr std::string_view alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64(const std::vector<uint8_t> &bytes) {
    std::string result;
    for(size_t i=0;i<bytes.size();i+=3) {
        const uint32_t a=bytes[i],b=i+1<bytes.size()?bytes[i+1]:0,c=i+2<bytes.size()?bytes[i+2]:0;
        result.push_back(alphabet[a>>2]);result.push_back(alphabet[((a&3)<<4)|(b>>4)]);
        result.push_back(i+1<bytes.size()?alphabet[((b&15)<<2)|(c>>6)]:'=');
        result.push_back(i+2<bytes.size()?alphabet[c&63]:'=');
    }
    return result;
}
std::vector<uint8_t> unbase64(std::string_view text) {
    if(text.empty() || text.size()%4!=0 || text.size()>kMaxHashBase64ComponentBytes)throw PasswordHashFormatError();
    std::vector<uint8_t> result;
    for(size_t i=0;i<text.size();i+=4) {
        uint32_t value=0;unsigned padding=0;
        for(size_t j=0;j<4;++j) {
            const auto c=text[i+j];
            if(c=='=') {if(j<2 || i+4!=text.size())throw PasswordHashFormatError();++padding;value<<=6;}
            else {const auto p=alphabet.find(c);if(p==std::string_view::npos||padding)throw PasswordHashFormatError();value=(value<<6)|static_cast<uint32_t>(p);}
        }
        result.push_back(static_cast<uint8_t>(value>>16));
        if(padding<2)result.push_back(static_cast<uint8_t>(value>>8));
        if(padding<1)result.push_back(static_cast<uint8_t>(value));
    }
    if(base64(result)!=text)throw PasswordHashFormatError();
    return result;
}
void check(const PasswordHash &p) {
    if(p.algorithm!="pbkdf2-sha256" || p.iterations==0 || p.iterations>kMaxStoredPasswordHashIterations ||
       p.salt.size()!=kPasswordSaltBytes || p.hash.size()!=kPasswordKeyBytes)throw PasswordHashFormatError();
}
}
std::string PasswordHash::encode() const {
    check(*this);
    return "$pbkdf2-sha256$i="+std::to_string(iterations)+"$"+base64(salt)+"$"+base64(hash);
}
PasswordHash PasswordHash::decode(std::string_view text) {
    constexpr std::string_view prefix="$pbkdf2-sha256$i=";
    if(!text.starts_with(prefix) || text.size()>kMaxEncodedPasswordHashBytes)throw PasswordHashFormatError();
    text.remove_prefix(prefix.size());const auto separator=text.find('$');
    if(separator==std::string_view::npos || separator==0 || text.front()=='0')throw PasswordHashFormatError();
    PasswordHash result;
    const auto number=text.substr(0,separator);
    const auto parsed=std::from_chars(number.data(),number.data()+number.size(),result.iterations);
    if(parsed.ec!=std::errc{} || parsed.ptr!=number.data()+number.size())throw PasswordHashFormatError();
    text.remove_prefix(separator+1);const auto split=text.find('$');
    if(split==std::string_view::npos)throw PasswordHashFormatError();
    result.salt=unbase64(text.substr(0,split));result.hash=unbase64(text.substr(split+1));check(result);return result;
}
}
