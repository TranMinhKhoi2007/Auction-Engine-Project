#pragma once
#include "auction/application/Dtos.h"
#include "auction/domain/ports/IUserRepository.h"
#include "auction/domain/values/PasswordPolicy.h"
#include "auction/domain/values/PasswordHash.h"
namespace auction::application {
struct RegistrationDraft {
    domain::Email email;
    std::string name;
};
struct CredentialRecord {
    common::UserId userId;
    domain::PasswordHash passwordHash;
};
class AccountService final {
public:
    explicit AccountService(domain::IUserRepository &users);
    [[nodiscard]] RegistrationDraft validateRegistration(std::string_view email,std::string_view name,std::string_view password) const;
    [[nodiscard]] AccountDto completeRegistration(const RegistrationDraft &,domain::PasswordHash);
    [[nodiscard]] std::optional<CredentialRecord> findCredentials(const domain::Email &) const;
    [[nodiscard]] AccountDto accountOf(common::UserId) const;
    void updatePasswordHash(common::UserId,domain::PasswordHash);
private:
    domain::IUserRepository &users_;
    uint64_t nextId_{1};
};
}
