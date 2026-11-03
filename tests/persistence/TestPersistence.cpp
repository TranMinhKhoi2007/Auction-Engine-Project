#include "auction/infrastructure_qt/SqliteStore.h"
#include "auction/infrastructure_qt/Pbkdf2PasswordHasher.h"
#include "auction/infrastructure_qt/QtEventLoopDriver.h"
#include "auction/application/AuctionEngine.h"
#include "auction/application/DefaultCommissionPolicy.h"
#include "auction/domain/values/EntityCheckpoint.h"
#include "tests/support/SimulatedClock.h"
#include "tests/support/SocketProbe.h"
#include "auction/protocol/JsonCodec.h"
#include "auction/server/ServerRuntime.h"
#include <QTest>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QProcess>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QFile>
using namespace auction;
using namespace auction::common;
using namespace auction::domain;
using namespace auction::application;
using namespace auction::infrastructure_qt;
namespace {
constexpr auto password="Copper!Lake839";
struct Fixture {
    std::shared_ptr<tests::SimulatedClock> clock;
    std::shared_ptr<SqliteStore> store;
    std::shared_ptr<SqlUserRepository> users;
    std::shared_ptr<SqlAuctionRepository> auctions;
    std::shared_ptr<infrastructure::EventLoop> loop;
    std::shared_ptr<Pbkdf2PasswordHasher> hasher;
    Fixture(const QString &path,WallTime wall=WallTime{Millis{1'792'108'800'000LL}}) {
        AuctionEngine::resetForTesting();clock=std::make_shared<tests::SimulatedClock>(wall);
        store=std::make_shared<SqliteStore>(path.toStdString(),*clock);
        users=std::make_shared<SqlUserRepository>(store);auctions=std::make_shared<SqlAuctionRepository>(store);
        loop=std::make_shared<infrastructure::EventLoop>(*clock);
        auto scheduler=std::make_shared<AuctionScheduler>(*loop,*clock,[](AuctionId id){AuctionEngine::instance().startAuction(id);},[](AuctionId id){AuctionEngine::instance().closeAuction(id);},store);
        auto service=std::make_shared<AuctionService>(*auctions,*users,*scheduler,std::make_shared<DefaultCommissionPolicy>(0),*clock);
        auto bidding=std::make_shared<BiddingService>(*auctions,*users,*clock,BidValidator::createDefault());hasher=std::make_shared<Pbkdf2PasswordHasher>(1000);
        AuctionEngine::initialize({clock,{},auctions,users,loop,scheduler,service,bidding,hasher,store});engine().recoverAuctions();
    }
    ~Fixture() {AuctionEngine::resetForTesting();}
    AuctionEngine &engine() {return AuctionEngine::instance();}
    UserId account(const char *email,const char *name="Tester") {return engine().registerUser({name,password,email}).id;}
    AuctionId auction(UserId seller,Seconds delay=Seconds{1},Seconds duration=Seconds{10},ProductType type=ProductType::DoCo) {
        CreateAuctionCommand c;c.sellerId=seller;c.productCommand.sellerId=seller;c.productCommand.type=type;
        c.productCommand.name="Bình gốm / tranh / máy";c.productCommand.description="Mô tả Unicode ' SQL";
        c.productCommand.eraCentury=15;c.productCommand.origin="Bát Tràng";c.productCommand.preservationState="Nguyên vẹn";c.productCommand.isAppraised=true;
        c.productCommand.artist="Họa sĩ";c.productCommand.yearCreated=2020;c.productCommand.medium="Sơn dầu";c.productCommand.dimensions="50x70";c.productCommand.hasCertificate=true;
        c.productCommand.brand="Brand";c.productCommand.model="Model";c.productCommand.warrantyMonths=24;c.productCommand.isNew=true;
        c.startingPrice=Money{100};c.minIncrement=Money{10};c.depositAmount=Money{100};c.startDelay=delay;c.duration=duration;
        return engine().createAuction(c);
    }
    void advance(Millis duration) {clock->advance(duration);loop->runDue();}
};
// Independent connection also verifies that acknowledged mutations reached SQLite.
QVariant sql(const QString &path,const QString &statement) {
    const auto name=QUuid::createUuid().toString();QVariant result;
    const auto cleanup=qScopeGuard([&]{QSqlDatabase::removeDatabase(name);});
    {
        auto db=QSqlDatabase::addDatabase("QSQLITE",name);db.setDatabaseName(path);
        if(!db.open())throw std::runtime_error(db.lastError().text().toStdString());
        QSqlQuery query(db);if(!query.exec(statement))throw std::runtime_error(query.lastError().text().toStdString());
        if(query.next())result=query.value(0);
    }
    return result;
}
struct Observer final : IAuctionObserver {
    int bids{},closed{},events{};
    std::function<void()> whenBid;
    void onAuctionEvent(const AuctionEvent &e) override {
        ++events;if(std::holds_alternative<BidPlacedEvent>(e)) {++bids;if(whenBid)whenBid();}
        if(std::holds_alternative<AuctionClosedEvent>(e))++closed;
    }
};
}
class TestPersistence final : public QObject {
    Q_OBJECT
private slots:
    void accountsProductsAndLargeMoney() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");
        UserId seller,buyer;std::vector<AuctionId> ids;std::vector<std::string> descriptions;
        constexpr int64_t huge=9'007'199'254'740'993LL;
        {
            Fixture f(path);seller=f.account("seller@sql.test","Người bán");buyer=f.account("buyer@sql.test");f.engine().topUp(buyer,Money{huge});
            for(const auto type:{ProductType::DoCo,ProductType::TranhNgheThuat,ProductType::ThietBiCongNghe}) {
                const auto id=f.auction(seller,Seconds{10},Seconds{20},type);ids.push_back(id);descriptions.push_back(f.auctions->findById(id)->item().describe());
            }
            f.engine().followAuction(ids[0],buyer);f.engine().approveAuction(ids[0],seller);f.engine().joinAuction(ids[0],buyer);
            QCOMPARE(sql(path,"SELECT available+held FROM wallets WHERE user_id=2").toLongLong(),huge);
            QVERIFY(!sql(path,"SELECT password_hash FROM identities WHERE user_id=2").toString().contains(password));
        }
        {
            Fixture f(path);QCOMPARE(f.engine().authenticate(" BUYER@SQL.TEST ",password).id,buyer);
            QCOMPARE(f.engine().getWallet(buyer)->total.amount(),huge);QCOMPARE(f.engine().getWallet(buyer)->totalHeld.amount(),100);
            QCOMPARE(f.engine().listUsers().size(),size_t{2});QCOMPARE(f.engine().listAuctions().size(),size_t{3});
            for(size_t i=0;i<ids.size();++i)QCOMPARE(f.auctions->findById(ids[i])->item().describe(),descriptions[i]);
            QVERIFY(f.users->findById(buyer)->isFollowing(ids[0]));QVERIFY(!f.engine().getInbox(buyer).empty());
            QCOMPARE(f.users->findById(seller)->createdAuctions().size(),size_t{3});
            QVERIFY(f.account("third@sql.test").value()>buyer.value());QVERIFY(f.auction(seller).value()>ids.back().value());
            QCOMPARE(sql(path,"PRAGMA integrity_check").toString(),QString("ok"));QVERIFY(sql(path,"PRAGMA foreign_key_check").isNull());
            QCOMPARE(sql(path,"SELECT count(*) FROM schema_migrations").toInt(),2);
        }
    }
    void scheduledActiveAndCountdownRestart() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s,b,c;AuctionId id;WallTime wall;
        {
            Fixture f(path);s=f.account("s@sql.test");b=f.account("b@sql.test");c=f.account("c@sql.test");f.engine().topUp(b,Money{1000});f.engine().topUp(c,Money{1000});id=f.auction(s,Seconds{5});
            f.engine().approveAuction(id,s);f.engine().joinAuction(id,b);f.engine().joinAuction(id,c);wall=f.clock->wallNow()+Seconds{3};
        }
        {
            Fixture f(path,wall);QCOMPARE(f.engine().getAuctionDetail(id)->state,std::string("Sắp diễn ra"));
            f.advance(Seconds{2});QCOMPARE(f.engine().getAuctionDetail(id)->state,std::string("Đang diễn ra"));
            f.engine().placeBid(id,b,Money{100});wall=f.clock->wallNow()+Seconds{2};
        }
        {
            Fixture f(path,wall);QCOMPARE(f.engine().getAuctionDetail(id)->state,std::string("Đang diễn ra"));
            const auto before=f.engine().getInbox(b).size();f.advance(Seconds{1});QVERIFY(f.engine().getInbox(b).size()>before);
            QVERIFY(std::holds_alternative<CountdownEvent>(f.engine().getInbox(b).back()));
            auto observer=std::make_shared<Observer>();f.engine().addGlobalObserver(observer);
            observer->whenBid=[&]{QCOMPARE(sql(path,"SELECT count(*) FROM bids").toInt(),2);};
            f.engine().placeBid(id,c,Money{110});QCOMPARE(observer->bids,1);
            f.advance(Seconds{8});QCOMPARE(f.engine().getAuctionDetail(id)->outcome,AuctionOutcome::Sold);QCOMPARE(observer->closed,1);
            QCOMPARE(f.engine().getWallet(s)->total.amount(),110);QCOMPARE(f.engine().getWallet(b)->total.amount(),1000);QCOMPARE(f.engine().getWallet(c)->total.amount(),890);
            QCOMPARE(f.engine().getAuctionDetail(id)->bidHistory.back().id().value(),uint64_t{2});
            wall=f.clock->wallNow();
        }
        const auto count=sql(path,"SELECT count(*) FROM ledger");
        {Fixture f(path,wall+Seconds{100});QCOMPARE(f.engine().getWallet(s)->total.amount(),110);}
        QCOMPARE(sql(path,"SELECT count(*) FROM ledger"),count);
    }
    void missedStartAndNoBidsClose() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s,b;AuctionId id;WallTime wall;
        {Fixture f(path);s=f.account("s@sql.test");b=f.account("b@sql.test");f.engine().topUp(b,Money{1000});id=f.auction(s);f.engine().approveAuction(id,s);f.engine().joinAuction(id,b);wall=f.clock->wallNow()+Seconds{30};}
        {Fixture f(path,wall);QCOMPARE(f.engine().getAuctionDetail(id)->outcome,AuctionOutcome::NoBids);QCOMPARE(f.engine().getWallet(b)->available.amount(),1000);QCOMPARE(f.engine().getWallet(b)->totalHeld.amount(),0);}
        const auto count=sql(path,"SELECT count(*) FROM ledger");
        {Fixture f(path,wall+Seconds{100});QCOMPARE(f.engine().getWallet(b)->available.amount(),1000);}
        QCOMPARE(sql(path,"SELECT count(*) FROM ledger"),count);
    }
    void missedStartRecoversActiveUntilOriginalDeadline() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s,b;AuctionId id;WallTime wall;
        {Fixture f(path);s=f.account("s@sql.test");b=f.account("b@sql.test");f.engine().topUp(b,Money{1000});id=f.auction(s,Seconds{5},Seconds{20});f.engine().approveAuction(id,s);f.engine().joinAuction(id,b);wall=f.clock->wallNow()+Seconds{7};}
        {Fixture f(path,wall);QCOMPARE(f.engine().getAuctionDetail(id)->state,std::string("Đang diễn ra"));f.advance(Seconds{17});
            QCOMPARE(f.engine().getAuctionDetail(id)->state,std::string("Đang diễn ra"));f.advance(Seconds{1});
            QCOMPARE(f.engine().getAuctionDetail(id)->outcome,AuctionOutcome::NoBids);QCOMPARE(f.engine().getWallet(b)->available.amount(),1000);}
    }
    void overdueClosuresUseChronologicalOrder() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s1,s2,b;AuctionId slow,fast;WallTime wall;
        {
            Fixture f(path);s1=f.account("one@sql.test");s2=f.account("two@sql.test");b=f.account("bidder@sql.test");f.engine().topUp(b,Money{1000});
            slow=f.auction(s1,Seconds{1},Seconds{20});fast=f.auction(s2,Seconds{1},Seconds{10});
            f.engine().approveAuction(slow,s1);f.engine().approveAuction(fast,s2);f.engine().joinAuction(slow,b);f.engine().joinAuction(fast,b);f.advance(Seconds{1});
            f.engine().placeBid(slow,b,Money{700});f.engine().placeBid(fast,b,Money{700});wall=f.clock->wallNow()+Seconds{30};
        }
        {Fixture f(path,wall);QCOMPARE(f.engine().getAuctionDetail(fast)->outcome,AuctionOutcome::Sold);QCOMPARE(f.engine().getAuctionDetail(slow)->outcome,AuctionOutcome::PaymentDefault);
            QCOMPARE(f.engine().getWallet(s2)->total.amount(),700);QCOMPARE(f.engine().getWallet(s1)->total.amount(),100);QCOMPARE(f.engine().getWallet(b)->total.amount(),200);
            QCOMPARE(sql(path,"SELECT count(*) FROM ledger WHERE kind='FORFEIT'").toInt(),1);
            QCOMPARE(sql(path,"SELECT sum(available_delta+held_delta) FROM ledger").toLongLong(),1000);
        }
    }
    void cancellationAndEarlyClosePersist() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s,b;AuctionId cancelled,early;
        {Fixture f(path);s=f.account("s@sql.test");b=f.account("b@sql.test");f.engine().topUp(b,Money{1000});cancelled=f.auction(s);early=f.auction(s);
            for(const auto id:{cancelled,early}) {f.engine().approveAuction(id,s);f.engine().joinAuction(id,b);}
            f.engine().cancelAuction(cancelled,s,"Hủy có hoàn cọc");f.advance(Seconds{1});f.engine().placeBid(early,b,Money{100});f.engine().closeAuctionEarly(early,s);}
        {Fixture f(path);QCOMPARE(f.engine().getAuctionDetail(cancelled)->state,std::string("Hủy"));QVERIFY(f.engine().getAuctionDetail(early)->closedEarly);
            QCOMPARE(f.engine().getWallet(b)->total.amount(),900);QCOMPARE(f.engine().getWallet(s)->total.amount(),100);QCOMPARE(f.engine().getWallet(b)->totalHeld.amount(),0);}
    }
    void failedSqlSettlementRollsBackEverything() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s,b;AuctionId id;
        {
            Fixture f(path);s=f.account("s@sql.test");b=f.account("b@sql.test");f.engine().topUp(b,Money{1000});id=f.auction(s);f.engine().approveAuction(id,s);f.engine().joinAuction(id,b);f.advance(Seconds{1});f.engine().placeBid(id,b,Money{100});
            auto observer=std::make_shared<Observer>();f.engine().addGlobalObserver(observer);const auto inbox=f.engine().getInbox(b).size();
            sql(path,"CREATE TRIGGER fail_settlement BEFORE INSERT ON ledger WHEN NEW.kind='SETTLE' BEGIN SELECT RAISE(ABORT,'injected storage failure'); END");
            QVERIFY_EXCEPTION_THROWN(f.engine().closeAuctionEarly(id,s),std::runtime_error);
            QCOMPARE(f.engine().getAuctionDetail(id)->state,std::string("Đang diễn ra"));QCOMPARE(f.engine().getInbox(b).size(),inbox);QCOMPARE(observer->events,0);
            QCOMPARE(f.engine().getWallet(b)->total.amount(),1000);QCOMPARE(f.engine().getWallet(b)->totalHeld.amount(),100);QCOMPARE(f.engine().getWallet(s)->total.amount(),0);
            QCOMPARE(sql(path,"SELECT state FROM auctions").toString(),QString("active"));QCOMPARE(sql(path,"SELECT count(*) FROM holds").toInt(),1);QCOMPARE(sql(path,"SELECT count(*) FROM ledger WHERE kind='SETTLE'").toInt(),0);
            QVERIFY_EXCEPTION_THROWN(f.engine().topUp(b,Money{1}),std::runtime_error); // fail closed after storage error
            sql(path,"DROP TRIGGER fail_settlement");
        }
        {Fixture f(path);f.engine().closeAuctionEarly(id,s);QCOMPARE(f.engine().getWallet(b)->total.amount(),900);}
    }
    void failedBidDoesNotPublishOrPersist() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");Fixture f(path);
        const auto s=f.account("s@sql.test"),b=f.account("b@sql.test");f.engine().topUp(b,Money{1000});const auto id=f.auction(s);f.engine().approveAuction(id,s);f.engine().joinAuction(id,b);f.advance(Seconds{1});
        auto observer=std::make_shared<Observer>();f.engine().addGlobalObserver(observer);const auto inbox=f.engine().getInbox(b).size();
        sql(path,"CREATE TRIGGER fail_bid BEFORE INSERT ON bids BEGIN SELECT RAISE(ABORT,'injected storage failure'); END");
        QVERIFY_EXCEPTION_THROWN(f.engine().placeBid(id,b,Money{100}),std::runtime_error);QCOMPARE(observer->bids,0);QCOMPARE(f.engine().getInbox(b).size(),inbox);
        QVERIFY(f.engine().getAuctionDetail(id)->bidHistory.empty());QCOMPARE(sql(path,"SELECT count(*) FROM bids").toInt(),0);
    }
    void failedAutomaticCloseStopsRetryAndRecoversNextRestart() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s,b;AuctionId id;WallTime wall;
        {
            Fixture f(path);s=f.account("s@sql.test");b=f.account("b@sql.test");f.engine().topUp(b,Money{1000});id=f.auction(s);
            f.engine().approveAuction(id,s);f.engine().joinAuction(id,b);f.advance(Seconds{1});f.engine().placeBid(id,b,Money{100});
            sql(path,"CREATE TRIGGER fail_close BEFORE INSERT ON ledger WHEN NEW.kind='SETTLE' BEGIN SELECT RAISE(ABORT,'injected storage failure'); END");
            f.advance(Seconds{10});QVERIFY(!f.store->isHealthy());QCOMPARE(f.engine().getAuctionDetail(id)->state,std::string("Đang diễn ra"));
            QVERIFY(!f.loop->nextEventTime());QCOMPARE(f.engine().getWallet(b)->totalHeld.amount(),100);
            wall=f.clock->wallNow();sql(path,"DROP TRIGGER fail_close");
        }
        {Fixture f(path,wall);QCOMPARE(f.engine().getAuctionDetail(id)->outcome,AuctionOutcome::Sold);QCOMPARE(f.engine().getWallet(b)->total.amount(),900);}
        QCOMPARE(sql(path,"SELECT count(*) FROM ledger WHERE kind='SETTLE'").toInt(),1);
    }
    void stoppedDriverDoesNotRearmOnScheduleChanges() {
        tests::SimulatedClock clock;infrastructure::EventLoop loop(clock);QtEventLoopDriver driver(loop,clock);int calls=0;
        driver.start();driver.stop();loop.schedule(clock.now(),[&]{++calls;});QTest::qWait(20);QCOMPARE(calls,0);
        driver.start();QTRY_COMPARE(calls,1);driver.stop();
    }
    void inboxCapFollowAndUnfollowSurviveRestart() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");UserId s,b,c;AuctionId id;WallTime wall;
        {Fixture f(path);s=f.account("s@sql.test");b=f.account("b@sql.test");c=f.account("c@sql.test");for(auto u:{b,c})f.engine().topUp(u,Money{10000});id=f.auction(s);f.engine().approveAuction(id,s);for(auto u:{b,c})f.engine().joinAuction(id,u);f.advance(Seconds{1});
            for(int i=0;i<210;++i)f.engine().placeBid(id,i%2?b:c,Money{100+i*10});
            f.engine().unfollowAuction(id,b);QCOMPARE(f.engine().getInbox(b).size(),kMaxInboxSize);wall=f.clock->wallNow();}
        {Fixture f(path,wall);QCOMPARE(f.engine().getInbox(b).size(),kMaxInboxSize);QVERIFY(!f.users->findById(b)->isFollowing(id));
            f.engine().placeBid(id,c,Money{2200});QVERIFY(std::holds_alternative<OutbidEvent>(f.engine().getInbox(b).back()));
            QCOMPARE(sql(path,"SELECT count(*) FROM notifications WHERE user_id=2").toInt(),int(kMaxInboxSize));
        }
    }
    void rejectsLockedNewerAndCorruptDatabase() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");
        {Fixture f(path);QVERIFY_EXCEPTION_THROWN(SqliteStore(path.toStdString(),*f.clock),std::runtime_error);}
        sql(path,"PRAGMA user_version=99");tests::SimulatedClock clock;
        QVERIFY_EXCEPTION_THROWN(SqliteStore(path.toStdString(),clock),std::runtime_error);
        const auto bad=dir.filePath("bad.sqlite");{QFile file(bad);QVERIFY(file.open(QIODevice::WriteOnly));file.write("not a sqlite database");}
        QVERIFY_EXCEPTION_THROWN(SqliteStore(bad.toStdString(),clock),std::runtime_error);
        const auto existing=dir.filePath("existing.sqlite");sql(existing,"CREATE TABLE keep_me(value INTEGER)");
        QVERIFY_EXCEPTION_THROWN(SqliteStore(existing.toStdString(),clock),std::runtime_error);
        QCOMPARE(sql(existing,"SELECT count(*) FROM sqlite_master WHERE name='keep_me'").toInt(),1);
    }
    void ledgerIsAppendOnlyAndReconciles() {
        QTemporaryDir dir;const auto path=dir.filePath("auction.sqlite");Fixture f(path);const auto b=f.account("b@sql.test");f.engine().topUp(b,Money{1000});f.engine().topUp(b,Money{25});
        QVERIFY_EXCEPTION_THROWN(sql(path,"UPDATE ledger SET available_delta=0"),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(sql(path,"DELETE FROM ledger"),std::runtime_error);
        QCOMPARE(sql(path,"SELECT sum(available_delta) FROM ledger").toLongLong(),1025);
        QCOMPARE(sql(path,"SELECT available FROM wallets").toLongLong(),1025);
    }
    void runtimeShutdownReleasesDatabaseBeforeApplicationExit() {
        QTemporaryDir dir;server::ServerConfig config;config.databasePath=dir.filePath("runtime.sqlite").toStdString();config.passwordHashIterations=1000;UserId user;
        {server::ServerRuntime runtime(dir.path().toStdString(),config);user=runtime.engine().registerUser({"Runtime user",password,"runtime@sql.test"}).id;runtime.engine().topUp(user,Money{1234});}
        QVERIFY_EXCEPTION_THROWN(AuctionEngine::instance(),std::logic_error);
        {server::ServerRuntime runtime(dir.path().toStdString(),config);QCOMPARE(runtime.engine().authenticate("runtime@sql.test",password).id,user);QCOMPARE(runtime.engine().getWallet(user)->available.amount(),1234);}
    }
    void productionServerSurvivesForcedKill() {
        QTemporaryDir dir;const auto path=dir.filePath("crash.sqlite");QProcess process;
        const auto cleanup=qScopeGuard([&]{if(process.state()!=QProcess::NotRunning){process.kill();process.waitForFinished(5000);}});
#ifdef Q_OS_WIN
        const auto executable=QCoreApplication::applicationDirPath()+"/AuctionServer.exe";
#else
        const auto executable=QCoreApplication::applicationDirPath()+"/AuctionServer";
#endif
        auto start=[&] {
            process.start(executable,{"--port","0","--log-dir",dir.path(),"--database",path});
            if(!process.waitForStarted(5000))throw std::runtime_error("Child server did not start");
            QByteArray output;
            if(!test::SocketProbe::wait([&]{output+=process.readAllStandardOutput();return output.contains("ws://127.0.0.1:");},30000))throw std::runtime_error(process.readAllStandardError().toStdString());
            const auto match=QRegularExpression("ws://127[.]0[.]0[.]1:([0-9]+)").match(QString::fromUtf8(output));
            return QUrl("ws://127.0.0.1:"+match.captured(1));
        };
        quint64 user{};double auction{};
        {test::SocketProbe p;p.timeoutMs=30000;p.open(start());QVERIFY(p.hello().value("ok").toBool());user=p.account("Crash buyer");QVERIFY(user>0);
            QVERIFY(p.call("wallet.topUp",{{"amount","9007199254740993"}}).value("ok").toBool());
            test::SocketProbe seller;seller.timeoutMs=30000;seller.open(p.socket.requestUrl());QVERIFY(seller.hello().value("ok").toBool());QVERIFY(seller.account("Crash seller")>0);
            CreateAuctionCommand command;command.productCommand.name="Crash recovery product";command.productCommand.description="Durable bid";
            command.startingPrice=Money{100};command.minIncrement=Money{10};command.depositAmount=Money{100};command.startDelay=Seconds{1};command.duration=Seconds{120};
            auction=seller.call("auction.create",protocol::toJson(command)).value("result").toObject().value("id").toDouble();QVERIFY(auction>0);
            const QJsonObject params{{"auctionId",auction}};QVERIFY(seller.call("auction.approve",params).value("ok").toBool());QVERIFY(p.call("auction.join",params).value("ok").toBool());
            QVERIFY(test::SocketProbe::wait([&]{return p.call("auction.get",params).value("result").toObject().value("state")=="DangDienRa";}));
            auto bid=params;bid.insert("amount","100");QVERIFY(p.call("auction.placeBid",bid).value("ok").toBool());
            process.kill();QVERIFY(process.waitForFinished(5000));}
        {test::SocketProbe p;p.timeoutMs=30000;p.open(start());QVERIFY(p.hello().value("ok").toBool());QVERIFY(p.login(user).value("ok").toBool());
            const auto wallet=p.call("wallet.get").value("result").toObject();QCOMPARE(wallet.value("total").toString(),QString("9007199254740993"));QCOMPARE(wallet.value("totalHeld").toString(),QString("100"));
            const auto detail=p.call("auction.get",{{"auctionId",auction}}).value("result").toObject();QCOMPARE(detail.value("bidHistory").toArray().size(),1);QCOMPARE(detail.value("currentPrice").toString(),QString("100"));}
        process.terminate();if(!process.waitForFinished(5000)){process.kill();process.waitForFinished(5000);}
        QCOMPARE(sql(path,"PRAGMA integrity_check").toString(),QString("ok"));QCOMPARE(sql(path,"SELECT count(*) FROM ledger").toInt(),2);QCOMPARE(sql(path,"SELECT count(*) FROM bids").toInt(),1);
    }
};
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);TestPersistence test;
    QFile::remove(QCoreApplication::applicationDirPath()+"/persistence-error.txt");
    try {return QTest::qExec(&test,argc,argv);}
    catch(const std::exception &e) {
        QFile diagnostic(QCoreApplication::applicationDirPath()+"/persistence-error.txt");
        if(diagnostic.open(QIODevice::WriteOnly|QIODevice::Truncate))diagnostic.write(e.what());
        qCritical("Persistence test exception: %s",e.what());return 1;
    }
}
#include "TestPersistence.moc"
