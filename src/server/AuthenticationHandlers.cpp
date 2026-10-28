#include "auction/server/RpcDispatcher.h"
#include "auction/server/ServerRuntime.h"
#include "auction/protocol/JsonCodec.h"
#include "auction/domain/values/SecretBuffer.h"
namespace auction::server {
using namespace protocol;
namespace {
struct PendingSubmission {
    ClientConnection &connection;bool accepted{};
    ~PendingSubmission(){if(!accepted)connection.authPending=false;}
};
bool active(const QPointer<ClientConnection> &c,quint64 epoch) {
    return c && c->socket && c->socket->state()==QAbstractSocket::ConnectedState && c->authEpoch==epoch;
}
}
void RpcDispatcher::limited(application::ThrottleResult result) {
    if(!result.allowed())throw ProtocolError("Thử lại sau ít giây","RateLimited",{},{{"retryAfterMs",result.retryAfter.count()}});
}
QJsonObject RpcDispatcher::createSession(ClientConnection &c,const application::AccountDto &account) {
    registry_.purgeExpired();
    auto created=runtime_.sessions().create(account.id,{c.clientName.toStdString(),c.address().toStdString()});
    domain::SecretBuffer wipe(created.token);registry_.apply(created.evicted);registry_.bind(c,created.session);
    runtime_.security().info("SESSION_CREATED user="+account.id.toString()+" address="+c.address().toStdString());
    return {{"account",toJson(account)},{"sessionToken",QString::fromStdString(created.token)},
        {"expiresAtMs",std::chrono::duration_cast<common::Millis>(created.expiresAt.time_since_epoch()).count()}};
}
void RpcDispatcher::registerAccount(ClientConnection &c,QJsonObject p,Completion done) {
    limited(runtime_.throttle().registrationAttempt(c.address().toStdString()));
    auto command=fromJson<application::RegisterUserCommand>(p);domain::SecretBuffer wipe(command.password);
    const auto draft=engine_.validateRegistration(command.email,command.name,command.password);
    const auto epoch=++c.authEpoch;QPointer<ClientConnection> guard(&c);c.authPending=true;PendingSubmission pending{c};
    pending.accepted=passwords_.hash(command.password,[this,guard,epoch,draft,done](PasswordResult result) {
        if(!active(guard,epoch))return;
        guard->authPending=false;
        try {
            if(result.error)std::rethrow_exception(result.error);
            if(!result.hash)throw std::logic_error("Missing password result");
            const auto account=engine_.completeRegistration(draft,std::move(*result.hash));
            runtime_.security().info("REGISTER user="+account.id.toString()+" address="+guard->address().toStdString());
            auto value=createSession(*guard,account);runtime_.security().info("LOGIN_OK user="+account.id.toString()+" address="+guard->address().toStdString());done(value,std::nullopt);
        } catch(const std::exception &e){done({},mapException(e,{0,"auth.register",{}}));}
          catch(...){logger_.error("Internal: registration continuation");done({},ErrorPayload{"Internal",{},"Lỗi hệ thống",{}});}
    });
    if(!pending.accepted)throw ProtocolError("Máy chủ đang bận, hãy thử lại","Busy");
}
void RpcDispatcher::login(ClientConnection &c,QJsonObject p,Completion done) {
    const auto input=readString(p.value("email")).toStdString();std::optional<domain::Email> email;
    try {email=domain::Email::parse(input);}catch(const domain::AccountValidationException &){}
    const auto key=email?email->value():input;const auto address=c.address().toStdString();
    const auto throttle=runtime_.throttle().check(key,address);
    if(!throttle.allowed()){
        runtime_.security().warn("LOGIN_FAIL reason=Locked email="+(email?email->masked():"<invalid>")+" address="+address);
        limited(throttle);
    }
    const auto record=email?engine_.findCredentials(*email):std::nullopt;
    auto password=readString(p.value("password")).toStdString();domain::SecretBuffer wipe(password);
    const auto stored=record?record->passwordHash:runtime_.dummyHash();
    const auto epoch=++c.authEpoch;QPointer<ClientConnection> guard(&c);c.authPending=true;PendingSubmission pending{c};
    pending.accepted=passwords_.verify(password,stored,record && stored.needsRehash(runtime_.config().passwordHashIterations),
        [this,guard,epoch,record,key,address,email,done](PasswordResult result) {
            if(!active(guard,epoch))return;
            guard->authPending=false;
            try {
                if(result.error)std::rethrow_exception(result.error);
                if(!record || !result.verified) {
                    runtime_.throttle().recordFailure(key,address);
                    const auto identity=record?"user="+record->userId.toString():"email="+(email?email->masked():"<invalid>");
                    runtime_.security().warn("LOGIN_FAIL reason=InvalidCredentials "+identity+" address="+address);
                    limited(runtime_.throttle().check(key,address));
                    throw ProtocolError("Email hoặc mật khẩu không đúng","Authentication","InvalidCredentials");
                }
                if(result.hash)engine_.updatePasswordHash(record->userId,std::move(*result.hash));
                runtime_.throttle().recordSuccess(key,address);
                const auto account=engine_.accountOf(record->userId);auto value=createSession(*guard,account);
                runtime_.security().info("LOGIN_OK user="+account.id.toString()+" address="+address);done(value,std::nullopt);
            } catch(const std::exception &e){done({},mapException(e,{0,"auth.login",{}}));}
              catch(...){logger_.error("Internal: login continuation");done({},ErrorPayload{"Internal",{},"Lỗi hệ thống",{}});}
        });
    if(!pending.accepted)throw ProtocolError("Máy chủ đang bận, hãy thử lại","Busy");
}
void RpcDispatcher::resetPassword(ClientConnection &c, QJsonObject p, Completion done) {
    const auto input = readString(p.value("email")).toStdString();
    std::optional<domain::Email> email;
    try { email = domain::Email::parse(input); } catch(const domain::AccountValidationException &) {}
    if (!email) throw ProtocolError("Email không hợp lệ", "Validation", "InvalidEmail");

    const auto record = engine_.findCredentials(*email);
    if (!record) throw ProtocolError("Không tìm thấy tài khoản với email này", "Validation", "EmailNotFound");

    auto password = readString(p.value("newPassword")).toStdString();
    domain::SecretBuffer wipe(password);
    if (password.size() < 8 || password.size() > 128) {
        throw ProtocolError("Mật khẩu mới cần từ 8 đến 128 ký tự", "Validation", "WeakPassword");
    }

    const auto epoch = ++c.authEpoch;
    QPointer<ClientConnection> guard(&c);
    c.authPending = true;
    PendingSubmission pending{c};

    pending.accepted = passwords_.hash(password, [this, guard, epoch, userId = record->userId, done](PasswordResult result) {
        try {
            if (result.error) std::rethrow_exception(result.error);
            if (!result.hash) throw std::logic_error("Missing password hash");
            engine_.updatePasswordHash(userId, std::move(*result.hash));
            runtime_.security().info("PASSWORD_RESET user=" + userId.toString());
        } catch (const std::exception &e) {
            if (active(guard, epoch)) {
                guard->authPending = false;
                done({}, mapException(e, {0, "auth.resetPassword", {}}));
            }
            return;
        } catch (...) {
            logger_.error("Internal: password reset continuation");
            if (active(guard, epoch)) {
                guard->authPending = false;
                done({}, ErrorPayload{"Internal", {}, "Lỗi hệ thống", {}});
            }
            return;
        }

        if (!active(guard, epoch)) return;
        guard->authPending = false;
        try {
            const auto account = engine_.accountOf(userId);
            auto value = createSession(*guard, account);
            done(value, std::nullopt);
        } catch (const std::exception &e) {
            done({}, mapException(e, {0, "auth.resetPassword", {}}));
        } catch (...) {
            logger_.error("Internal: password reset session creation");
            done({}, ErrorPayload{"Internal", {}, "Lỗi hệ thống", {}});
        }
    });
    if (!pending.accepted) throw ProtocolError("Máy chủ đang bận, hãy thử lại", "Busy");
}

void RpcDispatcher::changePassword(ClientConnection &c, const std::optional<AuthContext> &auth, QJsonObject p, Completion done) {
    if (!auth) throw ProtocolError("Cần đăng nhập để đổi mật khẩu", "Unauthorized");

    const auto currentPassword = readString(p.value("currentPassword")).toStdString();
    auto newPassword = readString(p.value("newPassword")).toStdString();
    domain::SecretBuffer wipeOld(const_cast<std::string&>(currentPassword));
    domain::SecretBuffer wipeNew(newPassword);

    if (newPassword.size() < 8 || newPassword.size() > 128) {
        throw ProtocolError("Mật khẩu mới cần từ 8 đến 128 ký tự", "Validation", "WeakPassword");
    }

    const auto userId = auth->userId;
    const auto account = engine_.accountOf(userId);
    const auto record = engine_.findCredentials(domain::Email::parse(account.email));
    if (!record) throw ProtocolError("Không tìm thấy tài khoản", "Validation", "UserNotFound");

    const auto epoch = ++c.authEpoch;
    QPointer<ClientConnection> guard(&c);
    c.authPending = true;
    PendingSubmission pending{c};

    pending.accepted = passwords_.verify(currentPassword, record->passwordHash, false, [this, guard, epoch, userId, newPassword, done](PasswordResult verifyRes) mutable {
        if (!active(guard, epoch)) return;
        if (verifyRes.error) {
            guard->authPending = false;
            done({}, mapException(std::logic_error("Lỗi xác thực"), {0, "auth.changePassword", {}}));
            return;
        }
        if (!verifyRes.verified) {
            guard->authPending = false;
            done({}, ErrorPayload{"Authentication", "InvalidCredentials", "Mật khẩu hiện tại không đúng", {}});
            return;
        }

        const bool queued = passwords_.hash(newPassword, [this, guard, epoch, userId, done](PasswordResult hashRes) {
            try {
                if (hashRes.error) std::rethrow_exception(hashRes.error);
                if (!hashRes.hash) throw std::logic_error("Missing password hash");
                engine_.updatePasswordHash(userId, std::move(*hashRes.hash));
                runtime_.security().info("PASSWORD_CHANGED user=" + userId.toString());
            } catch (const std::exception &e) {
                if (active(guard, epoch)) {
                    guard->authPending = false;
                    done({}, mapException(e, {0, "auth.changePassword", {}}));
                }
                return;
            } catch (...) {
                logger_.error("Internal: changePassword continuation");
                if (active(guard, epoch)) {
                    guard->authPending = false;
                    done({}, ErrorPayload{"Internal", {}, "Lỗi hệ thống", {}});
                }
                return;
            }

            if (!active(guard, epoch)) return;
            guard->authPending = false;
            QJsonObject res;
            res.insert("success", true);
            done(res, std::nullopt);
        });
        if (!queued) {
            if (active(guard, epoch)) {
                guard->authPending = false;
                done({}, ErrorPayload{"Busy", {}, "Máy chủ đang bận, hãy thử lại", {}});
            }
        }
    });
    if (!pending.accepted) throw ProtocolError("Máy chủ đang bận, hãy thử lại", "Busy");
}
}
