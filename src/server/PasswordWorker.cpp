#include "auction/server/PasswordWorker.h"
#include "auction/domain/values/SecretBuffer.h"
#include <QRunnable>
#include <algorithm>
namespace auction::server {
namespace {
struct Secret {
    explicit Secret(std::string_view input):value(input) {}
    ~Secret(){domain::SecretBuffer wipe(value);}
    std::string value;
};
}
PasswordWorker::PasswordWorker(std::shared_ptr<const domain::IPasswordHasher> hasher,QObject *parent,int threads,int queueLimit)
    :QObject(parent),hasher_(std::move(hasher)) {
    const int count=std::clamp(threads,1,domain::kPasswordWorkerThreads);
    pool_.setMaxThreadCount(count);
    pool_.setThreadPriority(QThread::LowPriority);
    maximumOutstanding_=count+std::clamp(queueLimit,0,domain::kPasswordWorkerQueueLimit);
    qRegisterMetaType<PasswordResultPtr>();
    connect(this,&PasswordWorker::completed,this,[this](quint64 id,PasswordResultPtr result) {
        auto callback=callbacks_.take(id);
        if(stopping_ || !callback)return;
        try {callback(std::move(*result));}
        catch(...) {qWarning("Password result continuation failed");}
    },Qt::QueuedConnection);
}
PasswordWorker::~PasswordWorker(){shutdown();}
bool PasswordWorker::hash(std::string_view plain,Callback callback){return submit(plain,std::nullopt,false,std::move(callback));}
bool PasswordWorker::verify(std::string_view plain,domain::PasswordHash stored,bool rehash,Callback callback) {
    return submit(plain,std::move(stored),rehash,std::move(callback));
}
bool PasswordWorker::submit(std::string_view plain,std::optional<domain::PasswordHash> stored,bool rehash,Callback callback) {
    if(stopping_ || callbacks_.size()>=maximumOutstanding_)return false;
    auto secret=std::make_shared<Secret>(plain);auto result=std::make_shared<PasswordResult>();
    const auto id=++nextId_;auto hasher=hasher_;callbacks_.insert(id,std::move(callback));
    pool_.start(QRunnable::create([this,id,hasher,secret,result,stored=std::move(stored),rehash] {
        {
            domain::SecretBuffer wipe(secret->value);
            try {
                if(stored) {
                    result->verified=hasher->verify(secret->value,*stored);
                    if(result->verified && rehash)result->hash=hasher->hash(secret->value);
                } else result->hash=hasher->hash(secret->value);
            } catch(...) {result->error=std::current_exception();}
        }
        // No engine, repository, session, connection or callback is accessed here.
        emit completed(id,result);
    }));
    return true;
}
void PasswordWorker::shutdown(){stopping_=true;pool_.clear();pool_.waitForDone();callbacks_.clear();}
void PasswordWorker::restart(){if(pool_.activeThreadCount()==0 && callbacks_.isEmpty())stopping_=false;}
}
