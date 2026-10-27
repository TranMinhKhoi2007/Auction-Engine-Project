#include "auction/protocol/JsonCodec.h"
#include "auction/common/ImageLimits.h"
#include "auction/protocol/ProtocolConfig.h"
#include <cmath>
#include <limits>
#include <QRegularExpression>
namespace auction::protocol {
QString imageBase64(const std::string &bytes) {
    return QString::fromLatin1(QByteArray(bytes.data(),static_cast<qsizetype>(bytes.size())).toBase64());
}
std::string readImageData(const QJsonValue &value) {
    if(value.isUndefined())return {}; // Older clients and auctions without an image.
    constexpr auto maximum=static_cast<qsizetype>((common::kMaxProductImageBytes+2)/3*4);
    if(!value.isString() || value.toString().size()>maximum)throw ProtocolError("Ảnh vượt giới hạn dung lượng","Validation","InvalidImage");
    const auto encoded=value.toString().toLatin1();
    const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
    if(!decoded || decoded.decoded.size()>static_cast<qsizetype>(common::kMaxProductImageBytes) || decoded.decoded.toBase64()!=encoded)
        throw ProtocolError("Dữ liệu ảnh không hợp lệ","Validation","InvalidImage");
    return decoded.decoded.toStdString();
}
qint64 readInteger(const QJsonValue &v) {
    if (!v.isDouble() || !std::isfinite(v.toDouble()) || std::floor(v.toDouble()) != v.toDouble() || std::abs(v.toDouble()) > 9007199254740991.0) throw ProtocolError("Số nguyên không hợp lệ");
    return static_cast<qint64>(v.toDouble());
}
quint64 readId(const QJsonValue &v) { const auto n=readInteger(v); if(n<0) throw ProtocolError("Id không hợp lệ"); return static_cast<quint64>(n); }
QString readString(const QJsonValue &v) { if(!v.isString() || v.toString().toUtf8().size()>kMaxStringBytes) throw ProtocolError("Chuỗi không hợp lệ"); return v.toString(); }
QJsonObject readObject(const QJsonValue &v) { if(!v.isObject()) throw ProtocolError("Thiếu đối tượng"); return v.toObject(); }
QJsonArray readArray(const QJsonValue &v) { if(!v.isArray()) throw ProtocolError("Thiếu danh sách"); return v.toArray(); }
domain::Money readMoney(const QJsonValue &v) {
    const auto s=readString(v); bool ok=false;
    const auto n=s.toLongLong(&ok);
    if(!ok || !QRegularExpression("^(0|[1-9][0-9]*)$").match(s).hasMatch()) throw ProtocolError("Tiền phải là chuỗi số thập phân không âm");
    return domain::Money(n);
}
namespace {
bool readBool(const QJsonValue &v) { if(!v.isBool()) throw ProtocolError("Thiếu boolean"); return v.toBool(); }
int readInt(const QJsonValue &v) { auto n=readInteger(v); if(n<std::numeric_limits<int>::min()||n>std::numeric_limits<int>::max()) throw ProtocolError("Số vượt phạm vi"); return static_cast<int>(n); }
QString string(const std::string &s) { return QString::fromStdString(s); }
qint64 epoch(common::WallTime t) { return std::chrono::duration_cast<common::Millis>(t.time_since_epoch()).count(); }
common::Seconds readSeconds(const QJsonValue &v) {
    const auto ms=readInteger(v);
    constexpr auto unit=std::chrono::duration_cast<common::Millis>(common::Seconds(1)).count();
    if(ms%unit!=0)throw ProtocolError("Thời lượng phải là số giây nguyên");
    return common::Seconds(ms/unit);
}
std::string readState(const QJsonValue &v) {
    const auto s=readString(v);
    if(s=="ChoDuyet") return "Chờ duyệt";
    if(s=="SapDienRa") return "Sắp diễn ra";
    if(s=="DangDienRa") return "Đang diễn ra";
    if(s=="KetThuc") return "Kết thúc";
    if(s=="Huy") return "Hủy";
    return "Unknown";
}
domain::AuctionOutcome readOutcome(const QJsonValue &v) {
    const auto s=readString(v);
    if(s=="None") return domain::AuctionOutcome::None;
    if(s=="Sold") return domain::AuctionOutcome::Sold;
    if(s=="NoBids") return domain::AuctionOutcome::NoBids;
    if(s=="PaymentDefault") return domain::AuctionOutcome::PaymentDefault;
    if(s=="Cancelled") return domain::AuctionOutcome::Cancelled;
    return static_cast<domain::AuctionOutcome>(-1); // Unknown sentinel without modifying the core enum.
}
application::ProductType readProduct(const QJsonValue &v) {
    const auto s=readString(v);
    if(s=="DoCo") return application::ProductType::DoCo;
    if(s=="TranhNgheThuat") return application::ProductType::TranhNgheThuat;
    if(s=="ThietBiCongNghe") return application::ProductType::ThietBiCongNghe;
    return static_cast<application::ProductType>(-1);
}
QString productName(application::ProductType t) {
    switch(t) {
    case application::ProductType::DoCo: return "DoCo";
    case application::ProductType::TranhNgheThuat: return "TranhNgheThuat";
    case application::ProductType::ThietBiCongNghe: return "ThietBiCongNghe";
    default: return "Unknown";
    }
}
}
QString stateName(const std::string &s) {
    if(s=="Chờ duyệt") return "ChoDuyet";
    if(s=="Sắp diễn ra") return "SapDienRa";
    if(s=="Đang diễn ra") return "DangDienRa";
    if(s=="Kết thúc") return "KetThuc";
    if(s=="Hủy") return "Huy";
    return "Unknown";
}
QString outcomeName(domain::AuctionOutcome t) {
    switch(t) {
    case domain::AuctionOutcome::None: return "None";
    case domain::AuctionOutcome::Sold: return "Sold";
    case domain::AuctionOutcome::NoBids: return "NoBids";
    case domain::AuctionOutcome::PaymentDefault: return "PaymentDefault";
    case domain::AuctionOutcome::Cancelled: return "Cancelled";
    default: return "Unknown";
    }
}
QJsonObject toJson(const application::UserDto &v) { return {{"id", static_cast<double>((v.id).value())}, {"name", string(v.name)}, {"availableBalance", QString::number((v.availableBalance).amount())}, {"heldBalance", QString::number((v.heldBalance).amount())}, {"totalBalance", QString::number((v.totalBalance).amount())}}; }
template<> application::UserDto fromJson(const QJsonValue &v) { const auto o=readObject(v); application::UserDto r;
r.id = common::UserId{readId(o.value("id"))};
r.name = readString(o.value("name")).toStdString();
r.availableBalance = readMoney(o.value("availableBalance"));
r.heldBalance = readMoney(o.value("heldBalance"));
r.totalBalance = readMoney(o.value("totalBalance"));
return r; }
QJsonObject toJson(const application::AuctionSummaryDto &v) { return {{"id", static_cast<double>((v.id).value())}, {"productName", string(v.productName)}, {"category", string(v.category)}, {"state", stateName(v.state)}, {"currentPrice", QString::number((v.currentPrice).amount())}, {"remainingTimeMs", (v.remainingTime).count()}}; }
template<> application::AuctionSummaryDto fromJson(const QJsonValue &v) { const auto o=readObject(v); application::AuctionSummaryDto r;
r.id = common::AuctionId{readId(o.value("id"))};
r.productName = readString(o.value("productName")).toStdString();
r.category = readString(o.value("category")).toStdString();
r.state = readState(o.value("state"));
r.currentPrice = readMoney(o.value("currentPrice"));
r.remainingTime = common::Millis(readInteger(o.value("remainingTimeMs")));
return r; }
QJsonObject toJson(const application::WalletHoldEntry &v) { return {{"auctionId", static_cast<double>((v.auctionId).value())}, {"auctionTitle", string(v.auctionTitle)}, {"amount", QString::number((v.amount).amount())}}; }
template<> application::WalletHoldEntry fromJson(const QJsonValue &v) { const auto o=readObject(v); application::WalletHoldEntry r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.auctionTitle = readString(o.value("auctionTitle")).toStdString();
r.amount = readMoney(o.value("amount"));
return r; }
QJsonObject toJson(const application::WalletDto &v) { return {{"available", QString::number((v.available).amount())}, {"totalHeld", QString::number((v.totalHeld).amount())}, {"total", QString::number((v.total).amount())}, {"holds", [&] { QJsonArray a; for(const auto &e : v.holds) a.append(toJson(e)); return a; }()}}; }
template<> application::WalletDto fromJson(const QJsonValue &v) { const auto o=readObject(v); application::WalletDto r;
r.available = readMoney(o.value("available"));
r.totalHeld = readMoney(o.value("totalHeld"));
r.total = readMoney(o.value("total"));
r.holds = [&] { std::vector<application::WalletHoldEntry> a; for(const auto &e : readArray(o.value("holds"))) a.push_back(fromJson<application::WalletHoldEntry>(e)); return a; }();
return r; }
QJsonObject toJson(const application::AuctionDetailDto &v) { return {{"hasImage", v.hasImage}, {"id", static_cast<double>((v.id).value())}, {"productId", static_cast<double>((v.productId).value())}, {"productName", string(v.productName)}, {"category", string(v.category)}, {"productDescription", string(v.productDescription)}, {"sellerId", static_cast<double>((v.sellerId).value())}, {"sellerName", string(v.sellerName)}, {"startingPrice", QString::number((v.startingPrice).amount())}, {"minIncrement", QString::number((v.minIncrement).amount())}, {"depositAmount", QString::number((v.depositAmount).amount())}, {"currentPrice", QString::number((v.currentPrice).amount())}, {"minimumNextBid", QString::number((v.minimumNextBid).amount())}, {"leaderId", (v.leaderId ? QJsonValue(static_cast<double>((*v.leaderId).value())) : QJsonValue(QJsonValue::Null))}, {"leaderName", string(v.leaderName)}, {"state", stateName(v.state)}, {"outcome", outcomeName(v.outcome)}, {"closedEarly", v.closedEarly}, {"startWallMs", (v.startWallTime ? QJsonValue(epoch(*v.startWallTime)) : QJsonValue(QJsonValue::Null))}, {"endWallMs", (v.endWallTime ? QJsonValue(epoch(*v.endWallTime)) : QJsonValue(QJsonValue::Null))}, {"startDelayMs", (v.startDelayMs).count()}, {"durationMs", (v.durationMs).count()}, {"remainingTimeMs", (v.remainingTime).count()}, {"participants", [&] { QJsonArray a; for(const auto &e : v.participants) a.append(static_cast<double>((e).value())); return a; }()}, {"bidHistory", [&] { QJsonArray a; for(const auto &e : v.bidHistory) a.append(toJson(e)); return a; }()}}; }
template<> application::AuctionDetailDto fromJson(const QJsonValue &v) { const auto o=readObject(v); application::AuctionDetailDto r;
r.id = common::AuctionId{readId(o.value("id"))};
r.productId = common::ProductId{readId(o.value("productId"))};
r.productName = readString(o.value("productName")).toStdString();
r.category = readString(o.value("category")).toStdString();
r.productDescription = readString(o.value("productDescription")).toStdString();
r.sellerId = common::UserId{readId(o.value("sellerId"))};
r.sellerName = readString(o.value("sellerName")).toStdString();
r.startingPrice = readMoney(o.value("startingPrice"));
r.minIncrement = readMoney(o.value("minIncrement"));
r.depositAmount = readMoney(o.value("depositAmount"));
r.currentPrice = readMoney(o.value("currentPrice"));
r.minimumNextBid = readMoney(o.value("minimumNextBid"));
r.leaderId = [&]() -> std::optional<common::UserId> { if(o.value("leaderId").isUndefined()) throw ProtocolError("Thiếu trường optional"); if(o.value("leaderId").isNull()) return std::nullopt; return common::UserId{readId(o.value("leaderId"))}; }();
r.leaderName = readString(o.value("leaderName")).toStdString();
r.state = readState(o.value("state"));
r.outcome = readOutcome(o.value("outcome"));
r.closedEarly = readBool(o.value("closedEarly"));
r.startWallTime = [&]() -> std::optional<common::WallTime> { if(o.value("startWallMs").isUndefined()) throw ProtocolError("Thiếu trường optional"); if(o.value("startWallMs").isNull()) return std::nullopt; return common::WallTime(common::Millis(readInteger(o.value("startWallMs")))); }();
r.endWallTime = [&]() -> std::optional<common::WallTime> { if(o.value("endWallMs").isUndefined()) throw ProtocolError("Thiếu trường optional"); if(o.value("endWallMs").isNull()) return std::nullopt; return common::WallTime(common::Millis(readInteger(o.value("endWallMs")))); }();
r.startDelayMs = common::Millis(readInteger(o.value("startDelayMs")));
r.durationMs = common::Millis(readInteger(o.value("durationMs")));
r.remainingTime = common::Millis(readInteger(o.value("remainingTimeMs")));
r.participants = [&] { std::vector<common::UserId> a; for(const auto &e : readArray(o.value("participants"))) a.push_back(common::UserId{readId(e)}); return a; }();
r.bidHistory = [&] { std::vector<domain::LuotTraGia> a; for(const auto &e : readArray(o.value("bidHistory"))) a.push_back(fromJson<domain::LuotTraGia>(e)); return a; }();
r.hasImage = o.value("hasImage").isUndefined()?false:readBool(o.value("hasImage"));
return r; }
QJsonObject toJson(const application::CreateProductCommand &v) { return {{"type", productName(v.type)}, {"name", string(v.name)}, {"description", string(v.description)}, {"sellerId", static_cast<double>((v.sellerId).value())}, {"eraCentury", v.eraCentury}, {"origin", string(v.origin)}, {"preservationState", string(v.preservationState)}, {"isAppraised", v.isAppraised}, {"artist", string(v.artist)}, {"yearCreated", v.yearCreated}, {"medium", string(v.medium)}, {"dimensions", string(v.dimensions)}, {"hasCertificate", v.hasCertificate}, {"brand", string(v.brand)}, {"model", string(v.model)}, {"warrantyMonths", v.warrantyMonths}, {"isNew", v.isNew}}; }
template<> application::CreateProductCommand fromJson(const QJsonValue &v) { const auto o=readObject(v); application::CreateProductCommand r;
r.type = readProduct(o.value("type"));
r.name = readString(o.value("name")).toStdString();
r.description = readString(o.value("description")).toStdString();
r.sellerId = common::UserId{readId(o.value("sellerId"))};
r.eraCentury = readInt(o.value("eraCentury"));
r.origin = readString(o.value("origin")).toStdString();
r.preservationState = readString(o.value("preservationState")).toStdString();
r.isAppraised = readBool(o.value("isAppraised"));
r.artist = readString(o.value("artist")).toStdString();
r.yearCreated = readInt(o.value("yearCreated"));
r.medium = readString(o.value("medium")).toStdString();
r.dimensions = readString(o.value("dimensions")).toStdString();
r.hasCertificate = readBool(o.value("hasCertificate"));
r.brand = readString(o.value("brand")).toStdString();
r.model = readString(o.value("model")).toStdString();
r.warrantyMonths = readInt(o.value("warrantyMonths"));
r.isNew = readBool(o.value("isNew"));
return r; }
QJsonObject toJson(const application::CreateAuctionCommand &v) { return {{"sellerId", static_cast<double>((v.sellerId).value())}, {"productCommand", toJson(v.productCommand)}, {"startingPrice", QString::number((v.startingPrice).amount())}, {"minIncrement", QString::number((v.minIncrement).amount())}, {"depositAmount", QString::number((v.depositAmount).amount())}, {"startDelayMs", std::chrono::duration_cast<common::Millis>(v.startDelay).count()}, {"durationMs", std::chrono::duration_cast<common::Millis>(v.duration).count()}, {"imageBase64", imageBase64(v.imageData)}}; }
template<> application::CreateAuctionCommand fromJson(const QJsonValue &v) { const auto o=readObject(v); application::CreateAuctionCommand r;
r.sellerId = common::UserId{readId(o.value("sellerId"))};
r.productCommand = fromJson<application::CreateProductCommand>(o.value("productCommand"));
r.startingPrice = readMoney(o.value("startingPrice"));
r.minIncrement = readMoney(o.value("minIncrement"));
r.depositAmount = readMoney(o.value("depositAmount"));
r.startDelay = readSeconds(o.value("startDelayMs"));
r.duration = readSeconds(o.value("durationMs"));
r.imageData = readImageData(o.value("imageBase64"));
return r; }
QJsonObject toJson(const application::AccountDto &v) {return {{"id",static_cast<double>(v.id.value())},{"name",string(v.name)},{"email",string(v.email)}};}
template<> application::AccountDto fromJson(const QJsonValue &v) {
    const auto o=readObject(v);return {common::UserId{readId(o.value("id"))},readString(o.value("name")).toStdString(),readString(o.value("email")).toStdString()};
}
QJsonObject toJson(const application::RegisterUserCommand &v) { return {{"name", string(v.name)}, {"password", string(v.password)},{"email",string(v.email)}}; }
template<> application::RegisterUserCommand fromJson(const QJsonValue &v) { const auto o=readObject(v); application::RegisterUserCommand r;
r.email=readString(o.value("email")).toStdString();
r.name = readString(o.value("name")).toStdString();
r.password = readString(o.value("password")).toStdString();
return r; }
QJsonObject toJson(const domain::BidPlacedEvent &v) { return {{"type", "BidPlacedEvent"}, {"auctionId", static_cast<double>((v.auctionId).value())}, {"bidderId", static_cast<double>((v.bidderId).value())}, {"amount", QString::number((v.amount).amount())}, {"timestampMs", epoch(v.timestamp)}}; }
template<> domain::BidPlacedEvent fromJson(const QJsonValue &v) { const auto o=readObject(v); domain::BidPlacedEvent r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.bidderId = common::UserId{readId(o.value("bidderId"))};
r.amount = readMoney(o.value("amount"));
r.timestamp = common::WallTime(common::Millis(readInteger(o.value("timestampMs"))));
return r; }
QJsonObject toJson(const domain::OutbidEvent &v) { return {{"type", "OutbidEvent"}, {"auctionId", static_cast<double>((v.auctionId).value())}, {"previousLeader", static_cast<double>((v.previousLeader).value())}, {"newAmount", QString::number((v.newAmount).amount())}, {"timestampMs", epoch(v.timestamp)}}; }
template<> domain::OutbidEvent fromJson(const QJsonValue &v) { const auto o=readObject(v); domain::OutbidEvent r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.previousLeader = common::UserId{readId(o.value("previousLeader"))};
r.newAmount = readMoney(o.value("newAmount"));
r.timestamp = common::WallTime(common::Millis(readInteger(o.value("timestampMs"))));
return r; }
QJsonObject toJson(const domain::ParticipantJoinedEvent &v) { return {{"type", "ParticipantJoinedEvent"}, {"auctionId", static_cast<double>((v.auctionId).value())}, {"userId", static_cast<double>((v.userId).value())}, {"deposit", QString::number((v.deposit).amount())}, {"timestampMs", epoch(v.timestamp)}}; }
template<> domain::ParticipantJoinedEvent fromJson(const QJsonValue &v) { const auto o=readObject(v); domain::ParticipantJoinedEvent r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.userId = common::UserId{readId(o.value("userId"))};
r.deposit = readMoney(o.value("deposit"));
r.timestamp = common::WallTime(common::Millis(readInteger(o.value("timestampMs"))));
return r; }
QJsonObject toJson(const domain::StateChangedEvent &v) { return {{"type", "StateChangedEvent"}, {"auctionId", static_cast<double>((v.auctionId).value())}, {"fromState", stateName(v.fromState)}, {"toState", stateName(v.toState)}, {"timestampMs", epoch(v.timestamp)}}; }
template<> domain::StateChangedEvent fromJson(const QJsonValue &v) { const auto o=readObject(v); domain::StateChangedEvent r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.fromState = readState(o.value("fromState"));
r.toState = readState(o.value("toState"));
r.timestamp = common::WallTime(common::Millis(readInteger(o.value("timestampMs"))));
return r; }
QJsonObject toJson(const domain::CountdownEvent &v) {
    if(v.secondsRemaining>9007199254740LL || v.secondsRemaining<-9007199254740LL) throw ProtocolError("Thời lượng vượt phạm vi JSON");
    return {{"type", "CountdownEvent"}, {"auctionId", static_cast<double>((v.auctionId).value())}, {"remainingMs",v.secondsRemaining*1000}, {"timestampMs", epoch(v.timestamp)}};
}
template<> domain::CountdownEvent fromJson(const QJsonValue &v) { const auto o=readObject(v); domain::CountdownEvent r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.secondsRemaining = readInteger(o.value("remainingMs"))/1000;
r.timestamp = common::WallTime(common::Millis(readInteger(o.value("timestampMs"))));
return r; }
QJsonObject toJson(const domain::AuctionClosedEvent &v) { return {{"type", "AuctionClosedEvent"}, {"auctionId", static_cast<double>((v.auctionId).value())}, {"outcome", outcomeName(v.outcome)}, {"winnerId", (v.winnerId ? QJsonValue(static_cast<double>((*v.winnerId).value())) : QJsonValue(QJsonValue::Null))}, {"finalPrice", QString::number((v.finalPrice).amount())}, {"timestampMs", epoch(v.timestamp)}, {"closedEarly", v.closedEarly}}; }
template<> domain::AuctionClosedEvent fromJson(const QJsonValue &v) { const auto o=readObject(v); domain::AuctionClosedEvent r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.outcome = readOutcome(o.value("outcome"));
r.winnerId = [&]() -> std::optional<common::UserId> { if(o.value("winnerId").isUndefined()) throw ProtocolError("Thiếu trường optional"); if(o.value("winnerId").isNull()) return std::nullopt; return common::UserId{readId(o.value("winnerId"))}; }();
r.finalPrice = readMoney(o.value("finalPrice"));
r.timestamp = common::WallTime(common::Millis(readInteger(o.value("timestampMs"))));
r.closedEarly = readBool(o.value("closedEarly"));
return r; }
QJsonObject toJson(const domain::AuctionCancelledEvent &v) { return {{"type", "AuctionCancelledEvent"}, {"auctionId", static_cast<double>((v.auctionId).value())}, {"reason", string(v.reason)}, {"timestampMs", epoch(v.timestamp)}}; }
template<> domain::AuctionCancelledEvent fromJson(const QJsonValue &v) { const auto o=readObject(v); domain::AuctionCancelledEvent r;
r.auctionId = common::AuctionId{readId(o.value("auctionId"))};
r.reason = readString(o.value("reason")).toStdString();
r.timestamp = common::WallTime(common::Millis(readInteger(o.value("timestampMs"))));
return r; }
QJsonObject toJson(const domain::LuotTraGia &v) {
 return {{"id",static_cast<double>(v.id().value())},{"auctionId",static_cast<double>(v.auctionId().value())},{"bidderId",static_cast<double>(v.bidderId().value())},{"amount",QString::number(v.amount().amount())},{"timestampMs",epoch(v.timestamp())}};
}
template<> domain::LuotTraGia fromJson(const QJsonValue &v) {
 const auto o=readObject(v);
 return {common::BidId{readId(o.value("id"))},common::AuctionId{readId(o.value("auctionId"))},common::UserId{readId(o.value("bidderId"))},readMoney(o.value("amount")),common::WallTime(common::Millis(readInteger(o.value("timestampMs"))))};
}
QJsonObject toJson(const domain::AuctionEvent &v) { return std::visit([](const auto &e){return toJson(e);},v); }
template<> domain::AuctionEvent fromJson(const QJsonValue &v) {
 const auto type=readString(readObject(v).value("type"));
if(type=="BidPlacedEvent") return fromJson<domain::BidPlacedEvent>(v);
if(type=="OutbidEvent") return fromJson<domain::OutbidEvent>(v);
if(type=="ParticipantJoinedEvent") return fromJson<domain::ParticipantJoinedEvent>(v);
if(type=="StateChangedEvent") return fromJson<domain::StateChangedEvent>(v);
if(type=="CountdownEvent") return fromJson<domain::CountdownEvent>(v);
if(type=="AuctionClosedEvent") return fromJson<domain::AuctionClosedEvent>(v);
if(type=="AuctionCancelledEvent") return fromJson<domain::AuctionCancelledEvent>(v);
throw ProtocolError("Event chưa hỗ trợ");
}
QJsonObject toJson(const ErrorPayload &e) { return {{"type",e.type},{"reason",e.reason.isEmpty()?QJsonValue(QJsonValue::Null):QJsonValue(e.reason)},{"message",e.message},{"data",e.data}}; }
template<> ErrorPayload fromJson(const QJsonValue &v) { const auto o=readObject(v); return {readString(o.value("type")),o.value("reason").isNull()?QString():readString(o.value("reason")),readString(o.value("message")),readObject(o.value("data"))}; }
}
