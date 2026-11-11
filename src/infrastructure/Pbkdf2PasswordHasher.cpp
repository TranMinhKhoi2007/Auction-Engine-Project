#include "auction/infrastructure/Pbkdf2PasswordHasher.h"
#include "auction/domain/AuctionConfig.h"
#include <limits>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

namespace auction::infrastructure {
namespace {
#ifdef _WIN32
void check(NTSTATUS status) {
    if (status < 0) {
        throw std::runtime_error("Không thể xử lý mật khẩu an toàn");
    }
}
struct Algorithm {
    BCRYPT_ALG_HANDLE handle{};
    explicit Algorithm(ULONG flags = BCRYPT_ALG_HANDLE_HMAC_FLAG) {
        check(BCryptOpenAlgorithmProvider(&handle, BCRYPT_SHA256_ALGORITHM, nullptr, flags));
    }
    ~Algorithm() { BCryptCloseAlgorithmProvider(handle, 0); }
    Algorithm(const Algorithm&) = delete;
    Algorithm& operator=(const Algorithm&) = delete;
};
#endif
std::vector<uint8_t> derive(std::string_view plain, const domain::PasswordHash& stored) {
    std::vector<uint8_t> result(32);
    if (plain.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("Mật khẩu quá dài");
    }
#ifdef _WIN32
    Algorithm algorithm;
    check(BCryptDeriveKeyPBKDF2(algorithm.handle,
        reinterpret_cast<PUCHAR>(const_cast<char*>(plain.data())), static_cast<ULONG>(plain.size()),
        const_cast<PUCHAR>(stored.salt.data()), static_cast<ULONG>(stored.salt.size()),
        stored.iterations, result.data(), static_cast<ULONG>(result.size()), 0));
#else
    if (PKCS5_PBKDF2_HMAC(plain.data(), static_cast<int>(plain.size()), stored.salt.data(),
            static_cast<int>(stored.salt.size()), static_cast<int>(stored.iterations),
            EVP_sha256(), static_cast<int>(result.size()), result.data()) != 1) {
        throw std::runtime_error("Không thể xử lý mật khẩu an toàn");
    }
#endif
    return result;
}
void wipe(std::vector<uint8_t>& bytes) noexcept {
#ifdef _WIN32
    SecureZeroMemory(bytes.data(), bytes.size());
#else
    OPENSSL_cleanse(bytes.data(), bytes.size());
#endif
}
} // namespace

domain::PasswordHash Pbkdf2PasswordHasher::hash(std::string_view plain) const {
    domain::PasswordHash result{std::vector<uint8_t>(16), {}, domain::kPasswordHashIterations};
#ifdef _WIN32
    check(BCryptGenRandom(nullptr, result.salt.data(), static_cast<ULONG>(result.salt.size()),
                         BCRYPT_USE_SYSTEM_PREFERRED_RNG));
#else
    if (RAND_bytes(result.salt.data(), static_cast<int>(result.salt.size())) != 1) {
        throw std::runtime_error("Không thể tạo salt an toàn");
    }
#endif
    result.hash = derive(plain, result);
    return result;
}

bool Pbkdf2PasswordHasher::verify(std::string_view plain, const domain::PasswordHash& stored) const {
    if (stored.salt.size() != 16 || stored.hash.size() != 32 ||
        stored.iterations != domain::kPasswordHashIterations) {
        return false;
    }
    auto candidate = derive(plain, stored);
    volatile uint8_t difference = 0;
    for (size_t i = 0; i < candidate.size(); ++i) {
        difference = static_cast<uint8_t>(difference | (candidate[i] ^ stored.hash[i]));
    }
    wipe(candidate);
    return difference == 0;
}

std::array<uint8_t, 32> Pbkdf2PasswordHasher::sha256(std::string_view input) {
    std::array<uint8_t, 32> output{};
#ifdef _WIN32
    Algorithm algorithm(0);
    BCRYPT_HASH_HANDLE hashHandle{};
    check(BCryptCreateHash(algorithm.handle, &hashHandle, nullptr, 0, nullptr, 0, 0));
    const auto status = BCryptHashData(hashHandle,
        reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())), static_cast<ULONG>(input.size()), 0);
    const auto finishStatus = status >= 0
        ? BCryptFinishHash(hashHandle, output.data(), static_cast<ULONG>(output.size()), 0) : status;
    BCryptDestroyHash(hashHandle);
    check(finishStatus);
#else
    unsigned int length{};
    if (EVP_Digest(input.data(), input.size(), output.data(), &length, EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("Không thể băm SHA-256");
    }
#endif
    return output;
}

} // namespace auction::infrastructure
