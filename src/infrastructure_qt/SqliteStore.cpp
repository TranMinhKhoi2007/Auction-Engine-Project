#include "auction/infrastructure_qt/SqliteStore.h"
#include "auction/application/ProductFactory.h"
#include "auction/domain/entities/DoCo.h"
#include "auction/domain/entities/TranhNgheThuat.h"
#include "auction/domain/entities/ThietBiCongNghe.h"
#include "auction/domain/values/EntityCheckpoint.h"
#include "auction/protocol/JsonCodec.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLockFile>
#include <QJsonDocument>
#include <QUuid>
#include <QThread>
#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>

static void initializeDatabaseResource() {Q_INIT_RESOURCE(database);}
namespace auction::infrastructure_qt {
namespace {
using namespace common;
using namespace domain;
class StorageError final : public std::runtime_error {
public:
    explicit StorageError(const std::string &message):std::runtime_error(message) {}
};
[[noreturn]] void sqlError(const QSqlError &e) {
    // Do not expose bound identities, credentials or SQL text to clients/logs.
    throw StorageError("SQLite không thể lưu/đọc dữ liệu (mã "+e.nativeErrorCode().toStdString()+")");
}
QVariant number(uint64_t id) {
    if(id>static_cast<uint64_t>(std::numeric_limits<qint64>::max()))throw StorageError("ID vượt giới hạn SQLite");
    return QVariant::fromValue(static_cast<qint64>(id));
}
QVariant integer(int64_t value) {return QVariant::fromValue(static_cast<qint64>(value));}
QString string(const std::string &s) {return QString::fromStdString(s);}
int64_t value(const QVariant &v) {
    bool ok=false;const auto result=v.toLongLong(&ok);
    if(!ok || (v.metaType().id()!=QMetaType::LongLong && v.metaType().id()!=QMetaType::Int))throw StorageError("Dữ liệu số SQLite không hợp lệ");
    return result;
}
uint64_t idValue(const QVariant &v) {const auto n=value(v);if(n<=0)throw StorageError("ID SQLite không hợp lệ");return static_cast<uint64_t>(n);}
qint64 epoch(WallTime t) {return std::chrono::duration_cast<Millis>(t.time_since_epoch()).count();}
QVariant timeValue(std::optional<WallTime> t) {return t?integer(epoch(*t)):QVariant{};}
std::optional<WallTime> timeFrom(const QVariant &v) {
    if(v.isNull())return {};
    const auto ms=value(v);
    // Keep conversions to system/steady nanoseconds within the supported epoch range.
    constexpr auto maxEpochMs=std::chrono::duration_cast<Millis>(WallTime::max().time_since_epoch()).count();
    if(ms<0 || ms>maxEpochMs)throw StorageError("Thời gian SQLite ngoài phạm vi hỗ trợ của clock");
    return WallTime{Millis{ms}};
}
QString stateCode(const std::string &s) {
    if(s=="Chờ duyệt")return "pending";
    if(s=="Sắp diễn ra")return "scheduled";
    if(s=="Đang diễn ra")return "active";
    if(s=="Kết thúc")return "closed";
    if(s=="Hủy")return "cancelled";
    throw StorageError("Trạng thái phiên không hợp lệ");
}
std::string stateText(const QString &s) {
    if(s=="pending")return "Chờ duyệt";
    if(s=="scheduled")return "Sắp diễn ra";
    if(s=="active")return "Đang diễn ra";
    if(s=="closed")return "Kết thúc";
    if(s=="cancelled")return "Hủy";
    throw StorageError("Trạng thái phiên trong SQLite không hợp lệ");
}
QString json(const QJsonObject &o) {return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));}
QJsonObject object(const QVariant &v) {
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(v.toString().toUtf8(),&error);
    if(error.error!=QJsonParseError::NoError || !doc.isObject())throw StorageError("Dữ liệu JSON trong SQLite không hợp lệ");
    return doc.object();
}
application::CreateProductCommand productCommand(const SanPhamDauGia &p) {
    application::CreateProductCommand c;c.name=p.name();c.description=p.description();c.sellerId=p.sellerId();
    if(const auto *a=dynamic_cast<const DoCo*>(&p)) {
        c.type=application::ProductType::DoCo;c.eraCentury=a->eraCentury();c.origin=a->origin();c.preservationState=a->preservationState();c.isAppraised=a->isAppraised();
    } else if(const auto *painting=dynamic_cast<const TranhNgheThuat*>(&p)) {
        c.type=application::ProductType::TranhNgheThuat;c.artist=painting->artist();c.yearCreated=painting->yearCreated();c.medium=painting->medium();c.dimensions=painting->dimensions();c.hasCertificate=painting->hasCertificate();
    } else if(const auto *device=dynamic_cast<const ThietBiCongNghe*>(&p)) {
        c.type=application::ProductType::ThietBiCongNghe;c.brand=device->brand();c.model=device->model();c.warrantyMonths=device->warrantyMonths();c.isNew=device->isNew();
    } else throw StorageError("Loại sản phẩm không được hỗ trợ");
    return c;
}
std::vector<QString> inboxJson(const std::deque<AuctionEvent> &events) {
    std::vector<QString> result;for(const auto &e:events)result.push_back(json(protocol::toJson(e)));return result;
}
}

struct SqliteStore::Impl {
    QSqlDatabase db;
    QString connectionName;
    std::unique_ptr<QLockFile> lock;
    const IClock &clock;
    QThread *thread{QThread::currentThread()};
    std::function<void()> onFailure;
    bool active{},healthy{true};
    std::map<UserId,std::shared_ptr<NguoiDung>> users;
    std::map<AuctionId,std::shared_ptr<PhienDauGia>> auctions;
    std::vector<std::function<void()>> pending;
    struct Deferred final : IAuctionObserver {
        Impl &owner;std::weak_ptr<IAuctionObserver> target;
        Deferred(Impl &o,std::weak_ptr<IAuctionObserver> t):owner(o),target(std::move(t)) {}
        void onAuctionEvent(const AuctionEvent &event) override {
            const auto deliver=[weak=target,event]{if(auto observer=weak.lock())observer->onAuctionEvent(event);};
            if(owner.active)owner.pending.push_back(deliver);else deliver();
        }
    };
    std::vector<std::shared_ptr<Deferred>> observers;
    explicit Impl(const IClock &c,std::function<void()> failure):clock(c),onFailure(std::move(failure)) {}
    ~Impl() {
        if(db.isValid())db.close();
        db=QSqlDatabase{};
        if(!connectionName.isEmpty())QSqlDatabase::removeDatabase(connectionName);
    }
    void checkThread() const {if(QThread::currentThread()!=thread)throw StorageError("SQLite phải được gọi trên luồng server");}
    QSqlQuery query(const QString &sql,const QVariantList &params={}) {
        QSqlQuery q(db);if(!q.prepare(sql))sqlError(q.lastError());
        for(const auto &p:params)q.addBindValue(p);
        if(!q.exec())sqlError(q.lastError());
        return q;
    }
    void open(const std::string &path) {
        initializeDatabaseResource();
        QString filename=string(path);
        if(filename!=":memory:") {
            QFileInfo info(filename);if(!QDir().mkpath(info.absolutePath()))throw StorageError("Không thể tạo thư mục dữ liệu SQLite");
            filename=info.absoluteFilePath();
            // Resolve existing symlinks before choosing the single-writer lock.
            if(info.exists())filename=info.canonicalFilePath();
            lock=std::make_unique<QLockFile>(filename+".lock");lock->setStaleLockTime(0);
            if(!lock->tryLock())throw StorageError("Database đang được một AuctionServer khác sử dụng");
        }
        connectionName="auction-sqlite-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        db=QSqlDatabase::addDatabase("QSQLITE",connectionName);db.setDatabaseName(filename);
        db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");if(!db.open())sqlError(db.lastError());
        query("PRAGMA foreign_keys=ON");query("PRAGMA journal_mode=WAL");query("PRAGMA synchronous=FULL");
        auto version=query("PRAGMA user_version");if(!version.next())throw StorageError("Không đọc được phiên bản SQLite");
        const auto v=value(version.value(0));
        version.finish();
        auto application=query("PRAGMA application_id");application.next();
        constexpr int applicationId=1096106836; // AuctionEngine database marker.
        if(v>2 || (v!=0 && value(application.value(0))!=applicationId))throw StorageError("Phiên bản database không được hỗ trợ");
        application.finish();
        if(v==0) {
            auto tables=query("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
            if(tables.next())throw StorageError("Database hiện có chưa có migration hợp lệ; không tự ghi đè");
            QFile migration(":/database/001_initial.sql");if(!migration.open(QIODevice::ReadOnly))throw StorageError("Thiếu migration SQLite");
            if(!db.transaction())sqlError(db.lastError());
            try {
                for(const auto &statement:QString::fromUtf8(migration.readAll()).split("--@statement"))if(!statement.trimmed().isEmpty())query(statement);
                query("INSERT INTO schema_migrations VALUES(1,?)",{integer(epoch(clock.wallNow()))});
                query("PRAGMA application_id=1096106836");query("PRAGMA user_version=1");
                if(!db.commit())sqlError(db.lastError());
            } catch(...) {db.rollback();throw;}
        }
        if(v<2) {
            QFile migration(":/database/002_product_images.sql");if(!migration.open(QIODevice::ReadOnly))throw StorageError("Thiếu migration ảnh SQLite");
            if(!db.transaction())sqlError(db.lastError());
            try {
                query(QString::fromUtf8(migration.readAll()));
                query("INSERT INTO schema_migrations VALUES(2,?)",{integer(epoch(clock.wallNow()))});
                query("PRAGMA user_version=2");
                if(!db.commit())sqlError(db.lastError());
            } catch(...) {db.rollback();throw;}
        }
        query("DROP TRIGGER IF EXISTS bids_no_delete");
        query("DROP TRIGGER IF EXISTS ledger_no_update");
        query("CREATE TRIGGER IF NOT EXISTS ledger_no_update BEFORE UPDATE OF transaction_id, user_id, kind, available_delta, held_delta, available_before, held_before, available_after, held_after ON ledger BEGIN SELECT RAISE(ABORT,'ledger is append only'); END;");
        auto integrity=query("PRAGMA quick_check");if(!integrity.next() || integrity.value(0).toString()!="ok")throw StorageError("Database SQLite bị hỏng");
        auto fk=query("PRAGMA foreign_key_check");if(fk.next())throw StorageError("Database SQLite có khóa ngoại không hợp lệ");
        load();
    }
    void load() {
        auto q=query("SELECT u.id,u.name,i.subject,i.password_hash,w.available,w.held FROM users u JOIN identities i ON i.user_id=u.id JOIN wallets w ON w.user_id=u.id ORDER BY u.id");
        while(q.next()) {
            const UserId id{idValue(q.value(0))};
            auto u=std::make_shared<NguoiDung>(id,q.value(1).toString().toStdString(),PasswordHash::decode(q.value(3).toString().toStdString()),Email::parse(q.value(2).toString().toStdString()));
            auto s=EntityCheckpoint::capture(*u);const Money available{value(q.value(4))},held{value(q.value(5))};
            s.wallet=ViTienDauGia(available+held);
            auto holds=query("SELECT auction_id,amount FROM holds WHERE user_id=? ORDER BY auction_id",{number(id.value())});
            while(holds.next())s.wallet.hold(AuctionId{idValue(holds.value(0))},Money{value(holds.value(1))});
            if(s.wallet.available()!=available || s.wallet.totalHeld()!=held)throw StorageError("Số dư và tiền cọc SQLite không khớp");
            auto follows=query("SELECT auction_id FROM follows WHERE user_id=? ORDER BY position",{number(id.value())});
            while(follows.next())s.followed.emplace_back(idValue(follows.value(0)));
            auto inbox=query("SELECT payload FROM notifications WHERE user_id=? ORDER BY id",{number(id.value())});
            while(inbox.next())s.inbox.push_back(protocol::fromJson<AuctionEvent>(object(inbox.value(0))));
            if(s.inbox.size()>kMaxInboxSize)throw StorageError("Hộp thông báo SQLite vượt giới hạn");
            auto last=query("SELECT available_after,held_after FROM ledger WHERE user_id=? ORDER BY id DESC LIMIT 1",{number(id.value())});
            if(last.next()) {
                if(value(last.value(0))!=available.amount() || value(last.value(1))!=held.amount())throw StorageError("Sổ cái và ví SQLite không khớp");
            } else if(available!=Money::zero() || held!=Money::zero())throw StorageError("Ví SQLite thiếu sổ cái");
            EntityCheckpoint::restore(*u,std::move(s));users.emplace(id,std::move(u));
        }
        auto count=query("SELECT count(*) FROM users");count.next();if(static_cast<size_t>(value(count.value(0)))!=users.size())throw StorageError("Tài khoản SQLite thiếu danh tính/ví");
        q=query("SELECT a.id,a.product_id,a.seller_id,a.starting_price,a.min_increment,a.deposit,a.start_delay_ms,a.duration_ms,a.state,a.start_at,a.end_at,a.outcome,a.closed_early,p.attributes FROM auctions a JOIN products p ON p.id=a.product_id ORDER BY a.id");
        while(q.next()) {
            const AuctionId id{idValue(q.value(0))};const UserId seller{idValue(q.value(2))};
            const auto c=protocol::fromJson<application::CreateProductCommand>(object(q.value(13)));
            if(c.sellerId!=seller || !users.contains(seller))throw StorageError("Người bán SQLite không hợp lệ");
            auto product=application::ProductFactory::createProduct(ProductId{idValue(q.value(1))},c);
            auto image=query("SELECT data FROM product_images WHERE product_id=?",{number(product->id().value())});
            if(image.next())product->setImageData(image.value(0).toByteArray().toStdString());
            auto a=std::make_shared<PhienDauGia>(id,std::move(product),seller,Money{value(q.value(3))},Money{value(q.value(4))},Money{value(q.value(5))},Millis{value(q.value(6))},Millis{value(q.value(7))});
            auto s=EntityCheckpoint::capture(*a);s.state=stateText(q.value(8).toString());s.startWall=timeFrom(q.value(9));s.endWall=timeFrom(q.value(10));
            if(s.startWall)s.start=toSteady(clock,*s.startWall);
            if(s.endWall)s.end=toSteady(clock,*s.endWall);
            s.outcome=static_cast<AuctionOutcome>(value(q.value(11)));s.closedEarly=q.value(12).toBool();
            auto participants=query("SELECT user_id FROM auction_participants WHERE auction_id=? ORDER BY position",{number(id.value())});
            while(participants.next())s.participants.emplace_back(idValue(participants.value(0)));
            auto bids=query("SELECT id,bidder_id,amount,placed_at FROM bids WHERE auction_id=? ORDER BY position",{number(id.value())});
            while(bids.next())s.bids.emplace_back(BidId{idValue(bids.value(0))},id,UserId{idValue(bids.value(1))},Money{value(bids.value(2))},*timeFrom(bids.value(3)));
            EntityCheckpoint::restore(*a,std::move(s));auctions.emplace(id,std::move(a));users.at(seller)->addAuction(id);
        }
    }
    struct Snapshot {
        std::map<UserId,std::shared_ptr<NguoiDung>> users;
        std::map<AuctionId,std::shared_ptr<PhienDauGia>> auctions;
        std::map<UserId,UserCheckpoint> userStates;
        std::map<AuctionId,AuctionCheckpoint> auctionStates;
        std::map<AuctionId,AuctionEventPublisher> publishers;
    };
    Snapshot snapshot() const {
        Snapshot s;s.users=users;s.auctions=auctions;
        for(const auto &[id,u]:users)s.userStates.emplace(id,EntityCheckpoint::capture(*u));
        for(const auto &[id,a]:auctions) {s.auctionStates.emplace(id,EntityCheckpoint::capture(*a));s.publishers.emplace(id,a->publisher());}
        return s;
    }
    void restore(Snapshot s) {
        users=std::move(s.users);auctions=std::move(s.auctions);
        for(auto &[id,state]:s.userStates)EntityCheckpoint::restore(*users.at(id),std::move(state));
        for(auto &[id,state]:s.auctionStates) {EntityCheckpoint::restore(*auctions.at(id),std::move(state));auctions.at(id)->publisher()=std::move(s.publishers.at(id));}
    }
    void flush(const Snapshot &before,const MutationContext &context) {
        // Parents first. Money, holds, bids, inbox and ledger commit atomically.
        for(const auto &[id,u]:users) {
            const bool fresh=!before.users.contains(id);
            if(fresh) {
                query("INSERT INTO users VALUES(?,?)",{number(id.value()),string(u->name())});
                query("INSERT INTO identities VALUES(?,'email',?,?)",{number(id.value()),string(u->email().value()),string(u->passwordHash().encode())});
            } else if(before.userStates.at(id).password.encode()!=u->passwordHash().encode())
                query("UPDATE identities SET password_hash=? WHERE user_id=?",{string(u->passwordHash().encode()),number(id.value())});
        }
        for(const auto &[id,a]:auctions) {
            const bool fresh=!before.auctions.contains(id);
            if(fresh) {
                const auto command=productCommand(a->item());
                query("INSERT INTO products VALUES(?,?,?,?,?,?)",{number(a->item().id().value()),number(a->sellerId().value()),string(a->item().category()),string(a->item().name()),string(a->item().description()),json(protocol::toJson(command))});
                if(!a->item().imageData().empty())query("INSERT INTO product_images VALUES(?,?)",{number(a->item().id().value()),QByteArray::fromStdString(a->item().imageData())});
                query("INSERT INTO auctions VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)",{number(id.value()),number(a->item().id().value()),number(a->sellerId().value()),stateCode(a->stateName()),integer(a->startingPrice().amount()),integer(a->minIncrement().amount()),integer(a->depositAmount().amount()),integer(a->startDelay().count()),integer(a->duration().count()),timeValue(a->startWallTime()),timeValue(a->endWallTime()),static_cast<int>(a->outcome()),a->closedEarly()});
            } else {
                const auto &old=before.auctionStates.at(id);
                if(old.state!=a->stateName() || old.startWall!=a->startWallTime() || old.endWall!=a->endWallTime() || old.outcome!=a->outcome() || old.closedEarly!=a->closedEarly())
                    query("UPDATE auctions SET state=?,start_at=?,end_at=?,outcome=?,closed_early=? WHERE id=?",{stateCode(a->stateName()),timeValue(a->startWallTime()),timeValue(a->endWallTime()),static_cast<int>(a->outcome()),a->closedEarly(),number(id.value())});
            }
            const size_t oldParticipants=fresh?0:before.auctionStates.at(id).participants.size();
            for(size_t i=oldParticipants;i<a->participants().size();++i)query("INSERT INTO auction_participants VALUES(?,?,?)",{number(id.value()),number(a->participants()[i].value()),number(i)});
            const size_t oldBids=fresh?0:before.auctionStates.at(id).bids.size();
            for(size_t i=oldBids;i<a->bidHistory().size();++i) {
                const auto &b=a->bidHistory()[i];
                query("INSERT INTO bids VALUES(?,?,?,?,?,?)",{number(b.id().value()),number(id.value()),number(b.bidderId().value()),integer(b.amount().amount()),integer(epoch(b.timestamp())),number(i)});
            }
        }
        for(const auto &[id,a]:before.auctions) {
            if(!auctions.contains(id)) {
                query("DELETE FROM notifications WHERE auction_id=?",{number(id.value())});
                query("UPDATE ledger SET auction_id=NULL WHERE auction_id=?",{number(id.value())});
                query("UPDATE transactions SET auction_id=NULL WHERE auction_id=?",{number(id.value())});
                query("DELETE FROM bids WHERE auction_id=?",{number(id.value())});
                query("DELETE FROM auction_participants WHERE auction_id=?",{number(id.value())});
                query("DELETE FROM holds WHERE auction_id=?",{number(id.value())});
                query("DELETE FROM follows WHERE auction_id=?",{number(id.value())});
                query("DELETE FROM product_images WHERE product_id=?",{number(a->item().id().value())});
                query("DELETE FROM auctions WHERE id=?",{number(id.value())});
                query("DELETE FROM products WHERE id=?",{number(a->item().id().value())});
            }
        }
        QVariant transactionId;
        for(const auto &[id,u]:users) {
            const bool fresh=!before.users.contains(id);
            const auto &wallet=u->wallet();
            const auto oldAvailable=fresh?Money::zero():before.userStates.at(id).wallet.available();
            const auto oldHeld=fresh?Money::zero():before.userStates.at(id).wallet.totalHeld();
            const bool changed=oldAvailable!=wallet.available() || oldHeld!=wallet.totalHeld();
            if(fresh)query("INSERT INTO wallets VALUES(?,?,?)",{number(id.value()),integer(wallet.available().amount()),integer(wallet.totalHeld().amount())});
            else if(changed)query("UPDATE wallets SET available=?,held=? WHERE user_id=?",{integer(wallet.available().amount()),integer(wallet.totalHeld().amount()),number(id.value())});
            const auto oldHolds=fresh?std::unordered_map<AuctionId,Money>{}:before.userStates.at(id).wallet.holds();
            for(const auto &[auction,amount]:oldHolds)if(!wallet.holds().contains(auction))query("DELETE FROM holds WHERE user_id=? AND auction_id=?",{number(id.value()),number(auction.value())});
            for(const auto &[auction,amount]:wallet.holds())if(!oldHolds.contains(auction) || oldHolds.at(auction)!=amount)
                query("INSERT INTO holds VALUES(?,?,?) ON CONFLICT(user_id,auction_id) DO UPDATE SET amount=excluded.amount",{number(id.value()),number(auction.value()),integer(amount.amount())});
            if(changed) {
                if(transactionId.isNull()) {
                    auto tx=query("INSERT INTO transactions(operation,actor_id,auction_id,committed_at) VALUES(?,?,?,?)",{string(context.operation),context.actor.value()?number(context.actor.value()):QVariant{},context.auction.value()?number(context.auction.value()):QVariant{},integer(epoch(clock.wallNow()))});transactionId=tx.lastInsertId();
                }
                QString kind=string(context.operation);
                if(context.operation=="CANCEL")kind="REFUND";
                if(context.operation=="CLOSE" || context.operation=="CLOSE_EARLY") {
                    const auto &a=auctions.at(context.auction);
                    if(id==a->sellerId())kind="PROCEEDS";
                    else if(a->highestBid() && a->highestBid()->bidderId()==id)kind=a->outcome()==AuctionOutcome::PaymentDefault?"FORFEIT":"SETTLE";
                    else kind="REFUND";
                }
                query("INSERT INTO ledger(transaction_id,user_id,auction_id,kind,available_delta,held_delta,available_before,held_before,available_after,held_after) VALUES(?,?,?,?,?,?,?,?,?,?)",{transactionId,number(id.value()),context.auction.value()?number(context.auction.value()):QVariant{},kind,integer(wallet.available().amount()-oldAvailable.amount()),integer(wallet.totalHeld().amount()-oldHeld.amount()),integer(oldAvailable.amount()),integer(oldHeld.amount()),integer(wallet.available().amount()),integer(wallet.totalHeld().amount())});
            }
            const auto followed=u->followedAuctions();
            const auto oldFollowed=fresh?std::vector<AuctionId>{}:before.userStates.at(id).followed;
            if(!std::equal(followed.begin(),followed.end(),oldFollowed.begin(),oldFollowed.end())) {
                query("DELETE FROM follows WHERE user_id=?",{number(id.value())});
                for(size_t i=0;i<followed.size();++i)
                    if(auctions.contains(followed[i]))
                        query("INSERT INTO follows VALUES(?,?,?)",{number(id.value()),number(followed[i].value()),number(i)});
            }
            const auto oldInbox=fresh?std::vector<QString>{}:inboxJson(before.userStates.at(id).inbox);
            const auto newInbox=inboxJson(u->inbox());
            if(oldInbox!=newInbox) {
                size_t overlap=std::min(oldInbox.size(),newInbox.size());
                while(overlap && !std::equal(oldInbox.end()-static_cast<ptrdiff_t>(overlap),oldInbox.end(),newInbox.begin()))--overlap;
                const auto removed=oldInbox.size()-overlap;
                if(removed)query("DELETE FROM notifications WHERE id IN (SELECT id FROM notifications WHERE user_id=? ORDER BY id LIMIT ?)",{number(id.value()),number(removed)});
                for(size_t i=overlap;i<newInbox.size();++i) {
                    const auto auction=std::visit([](const auto &e){return e.auctionId;},u->inbox()[i]);
                    if(auctions.contains(auction))
                        query("INSERT INTO notifications(user_id,auction_id,payload) VALUES(?,?,?)",{number(id.value()),number(auction.value()),newInbox[i]});
                }
            }
        }
    }
};

SqliteStore::SqliteStore(const std::string &path,const IClock &clock,std::function<void()> failure):impl_(std::make_unique<Impl>(clock,std::move(failure))) {impl_->open(path);}
SqliteStore::~SqliteStore()=default;
bool SqliteStore::isHealthy() const noexcept {return impl_->healthy;}
void SqliteStore::execute(const MutationContext &context,const std::function<void()> &action) {
    auto &s=*impl_;s.checkThread();
    if(!s.healthy)throw StorageError("Database đã gặp lỗi; hãy khởi động lại server sau khi kiểm tra ổ đĩa");
    if(s.active)throw StorageError("Không hỗ trợ transaction lồng nhau");
    auto before=s.snapshot();s.active=true;s.pending.clear();
    try {
        if(!s.db.transaction())sqlError(s.db.lastError());
        action();s.flush(before,context);
        if(!s.db.commit()) {
            sqlError(s.db.lastError());
        }
        s.query("PRAGMA wal_checkpoint(PASSIVE)");
    } catch(...) {
        const auto error=std::current_exception();s.db.rollback();s.restore(std::move(before));s.pending.clear();s.active=false;
        try {std::rethrow_exception(error);}catch(const StorageError &) {s.healthy=false;if(s.onFailure)s.onFailure();throw;}catch(...) {throw;}
    }
    s.active=false;auto deliveries=std::move(s.pending);s.pending.clear();
    for(const auto &deliver:deliveries)try {deliver();}catch(...) { /* A committed mutation cannot be rolled back by an observer. */ }
}
std::weak_ptr<IAuctionObserver> SqliteStore::afterCommit(std::weak_ptr<IAuctionObserver> target) {
    auto proxy=std::make_shared<Impl::Deferred>(*impl_,std::move(target));impl_->observers.push_back(proxy);return proxy;
}
void SqlUserRepository::save(std::shared_ptr<NguoiDung> u) {
    auto &s=*store_->impl_;s.checkThread();if(!s.active)throw StorageError("Lưu tài khoản cần transaction");
    s.users.insert_or_assign(u->id(),std::move(u));
}
std::shared_ptr<NguoiDung> SqlUserRepository::findById(UserId id) const {
    const auto &s=*store_->impl_;s.checkThread();const auto it=s.users.find(id);return it==s.users.end()?nullptr:it->second;
}
std::vector<std::shared_ptr<NguoiDung>> SqlUserRepository::findAll() const {
    const auto &s=*store_->impl_;s.checkThread();std::vector<std::shared_ptr<NguoiDung>> result;for(const auto &[id,u]:s.users)result.push_back(u);return result;
}
std::shared_ptr<NguoiDung> SqlUserRepository::findByEmail(const Email &email) const {for(const auto &u:findAll())if(u->email()==email)return u;return {};}
bool SqlUserRepository::emailExists(const Email &email) const {return static_cast<bool>(findByEmail(email));}
void SqlAuctionRepository::save(std::shared_ptr<PhienDauGia> a) {
    auto &s=*store_->impl_;s.checkThread();if(!s.active)throw StorageError("Lưu phiên cần transaction");s.auctions.insert_or_assign(a->id(),std::move(a));
}
bool SqlAuctionRepository::remove(AuctionId id) {
    auto &s=*store_->impl_;s.checkThread();if(!s.active)throw StorageError("Xóa phiên cần transaction");return s.auctions.erase(id)>0;
}
std::shared_ptr<PhienDauGia> SqlAuctionRepository::findById(AuctionId id) const {
    const auto &s=*store_->impl_;s.checkThread();const auto it=s.auctions.find(id);return it==s.auctions.end()?nullptr:it->second;
}
std::vector<std::shared_ptr<PhienDauGia>> SqlAuctionRepository::findAll() const {
    const auto &s=*store_->impl_;s.checkThread();std::vector<std::shared_ptr<PhienDauGia>> result;for(const auto &[id,a]:s.auctions)result.push_back(a);return result;
}
}
