#include "auction/infrastructure_qt/Pbkdf2PasswordHasher.h"
#include "auction/infrastructure_qt/OsTokenGenerator.h"
#include <QPasswordDigestor>
#include <QCryptographicHash>
#include <limits>
#include <stdexcept>
namespace auction::infrastructure_qt {
namespace {
struct WipeBytes {
    QByteArray &data;
    ~WipeBytes() {volatile char *p=data.data();for(qsizetype i=0;i<data.size();++i)p[i]=0;data.clear();}
};
}
Pbkdf2PasswordHasher::Pbkdf2PasswordHasher(uint32_t iterations):iterations_(iterations) {
    if(iterations==0 || iterations>domain::kMaxStoredPasswordHashIterations)throw std::invalid_argument("Invalid PBKDF2 iterations");
}
std::vector<uint8_t> Pbkdf2PasswordHasher::derive(std::string_view plain,std::span<const uint8_t> salt,uint32_t iterations) {
    if(iterations==0 || iterations>domain::kMaxStoredPasswordHashIterations ||
       plain.size()>static_cast<size_t>(std::numeric_limits<qsizetype>::max()) || salt.size()>static_cast<size_t>(std::numeric_limits<qsizetype>::max()))
        throw std::invalid_argument("Invalid PBKDF2 parameters");
    QByteArray input(plain.data(),static_cast<qsizetype>(plain.size()));WipeBytes wipeInput{input};
    const auto saltBytes=QByteArray::fromRawData(reinterpret_cast<const char *>(salt.data()),static_cast<qsizetype>(salt.size()));
    auto key=QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,input,saltBytes,static_cast<int>(iterations),domain::kPasswordKeyBytes);
    WipeBytes wipeKey{key};
    if(key.size()!=static_cast<qsizetype>(domain::kPasswordKeyBytes))throw std::runtime_error("PBKDF2 failed");
    return {key.begin(),key.end()};
}
domain::PasswordHash Pbkdf2PasswordHasher::hash(std::string_view plain) const {
    domain::PasswordHash result{OsTokenGenerator::randomBytes(domain::kPasswordSaltBytes),{},iterations_};
    result.hash=derive(plain,result.salt,iterations_);return result;
}
bool Pbkdf2PasswordHasher::verify(std::string_view plain,const domain::PasswordHash &stored) const {
    if(stored.algorithm!="pbkdf2-sha256" || stored.salt.size()!=domain::kPasswordSaltBytes ||
       stored.hash.size()!=domain::kPasswordKeyBytes || stored.iterations==0 || stored.iterations>domain::kMaxStoredPasswordHashIterations)return false;
    auto candidate=derive(plain,stored.salt,stored.iterations);
    volatile unsigned difference=0;
    for(size_t i=0;i<candidate.size();++i)difference=static_cast<unsigned>(difference)|(static_cast<unsigned>(candidate[i]^stored.hash[i]));
    volatile uint8_t *bytes=candidate.data();for(size_t i=0;i<candidate.size();++i)bytes[i]=0;
    return difference==0;
}
}
