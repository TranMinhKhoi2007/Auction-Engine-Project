#include "auction/domain/values/EntityCheckpoint.h"
#include "auction/domain/state/ChoDuyetState.h"
#include "auction/domain/state/SapDienRaState.h"
#include "auction/domain/state/DangDienRaState.h"
#include "auction/domain/state/KetThucState.h"
#include "auction/domain/state/HuyState.h"
#include <stdexcept>
namespace auction::domain {
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
UserCheckpoint EntityCheckpoint::capture(const NguoiDung &u) {
    return {u.wallet_,u.passwordHash_,u.postedAuctions_,u.followedAuctions_,u.inbox_};
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
AuctionCheckpoint EntityCheckpoint::capture(const PhienDauGia &a) {
    return {a.stateName(),a.startTime_,a.endTime_,a.startWallTime_,a.endWallTime_,a.bids_,a.participants_,a.outcome_,a.closedEarly_};
}
void EntityCheckpoint::restore(NguoiDung &u,UserCheckpoint s) {
    u.wallet_=std::move(s.wallet);u.passwordHash_=std::move(s.password);
    u.postedAuctions_=std::move(s.created);u.followedAuctions_=std::move(s.followed);u.inbox_=std::move(s.inbox);
}
void EntityCheckpoint::restore(PhienDauGia &a,AuctionCheckpoint s) {
    std::unique_ptr<IAuctionState> state;
    if(s.state=="Chờ duyệt")state=std::make_unique<ChoDuyetState>();
    else if(s.state=="Sắp diễn ra")state=std::make_unique<SapDienRaState>();
    else if(s.state=="Đang diễn ra")state=std::make_unique<DangDienRaState>();
    else if(s.state=="Kết thúc")state=std::make_unique<KetThucState>();
    else if(s.state=="Hủy")state=std::make_unique<HuyState>();
    else throw std::invalid_argument("Trạng thái lưu trữ không hợp lệ");
    // Restore silently. Keep the publisher and shared entity identity intact.
    a.state_=std::move(state);a.startTime_=s.start;a.endTime_=s.end;
    a.startWallTime_=s.startWall;a.endWallTime_=s.endWall;a.bids_=std::move(s.bids);
    a.participants_=std::move(s.participants);a.outcome_=s.outcome;a.closedEarly_=s.closedEarly;
}
}
