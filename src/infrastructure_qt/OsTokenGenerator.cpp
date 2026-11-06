#include "auction/infrastructure_qt/OsTokenGenerator.h"
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QByteArray>
#include <limits>
#include <stdexcept>
#include <algorithm>
namespace auction::infrastructure_qt {
std::vector<uint8_t> OsTokenGenerator::randomBytes(size_t count) {
    std::vector<uint8_t> bytes(count);
    for(size_t i=0;i<count;) {
        const auto word=QRandomGenerator::system()->generate();
        for(unsigned shift=0;shift<32 && i<count;shift+=8)bytes[i++]=static_cast<uint8_t>(word>>shift);
    }
    return bytes;
}
std::string OsTokenGenerator::generate(size_t count) const {
    if(count==0 || count>static_cast<size_t>(std::numeric_limits<qsizetype>::max()))throw std::invalid_argument("Invalid random byte count");
    auto bytes=randomBytes(count);
    auto encoded=QByteArray::fromRawData(reinterpret_cast<const char *>(bytes.data()),static_cast<qsizetype>(bytes.size())).toBase64(QByteArray::Base64UrlEncoding|QByteArray::OmitTrailingEquals);
    std::string result(encoded.constData(),static_cast<size_t>(encoded.size()));
    volatile uint8_t *data=bytes.data();for(size_t i=0;i<bytes.size();++i)data[i]=0;
    volatile char *text=encoded.data();for(qsizetype i=0;i<encoded.size();++i)text[i]=0;
    return result;
}
domain::ITokenGenerator::Digest OsTokenGenerator::digest(std::string_view token) const {
    if(token.size()>static_cast<size_t>(std::numeric_limits<qsizetype>::max()))throw std::invalid_argument("Invalid token size");
    const auto hash=QCryptographicHash::hash(QByteArray::fromRawData(token.data(),static_cast<qsizetype>(token.size())),QCryptographicHash::Sha256);
    Digest result{};std::copy(hash.begin(),hash.end(),result.begin());return result;
}
}
