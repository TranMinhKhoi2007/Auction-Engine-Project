#pragma once
#include "auction/domain/entities/NguoiDung.h"
#include "auction/domain/entities/PhienDauGia.h"
namespace auction::domain {
// STL-only hydration/checkpoint data. Never a network DTO.
struct UserCheckpoint {
    ViTienDauGia wallet;
    PasswordHash password;
    std::vector<common::AuctionId> created,followed;
    std::deque<AuctionEvent> inbox;
};
struct AuctionCheckpoint {
    std::string state;
    std::optional<common::SteadyTime> start,end;
    std::optional<common::WallTime> startWall,endWall;
    std::vector<LuotTraGia> bids;
    std::vector<common::UserId> participants;
    AuctionOutcome outcome{AuctionOutcome::None};
    bool closedEarly{};
};
class EntityCheckpoint final {
public:
    static UserCheckpoint capture(const NguoiDung &);
    static AuctionCheckpoint capture(const PhienDauGia &);
    static void restore(NguoiDung &,UserCheckpoint);
    static void restore(PhienDauGia &,AuctionCheckpoint);
};
}
