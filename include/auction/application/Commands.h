#pragma once

#include "auction/common/StrongId.h"
#include "auction/common/TimeTypes.h"
#include "auction/domain/values/Money.h"
#include <string>

namespace auction::application {

enum class ProductType {
    DoCo,
    TranhNgheThuat,
    ThietBiCongNghe
};

struct CreateProductCommand {
    ProductType type{ProductType::DoCo};
    std::string name;
    std::string description;
    common::UserId sellerId;

    // Trường cho Đồ cổ
    int eraCentury{0};
    std::string origin;
    std::string preservationState;
    bool isAppraised{false};

    // Trường cho Tranh nghệ thuật
    std::string artist;
    int yearCreated{0};
    std::string medium;
    std::string dimensions;
    bool hasCertificate{false};

    // Trường cho Thiết bị công nghệ
    std::string brand;
    std::string model;
    int warrantyMonths{0};
    bool isNew{false};
};

struct CreateAuctionCommand {
    common::UserId sellerId;
    CreateProductCommand productCommand;
    domain::Money startingPrice{domain::Money::zero()};
    domain::Money minIncrement{domain::Money::zero()};
    domain::Money depositAmount{domain::Money::zero()};
    common::Seconds startDelay{common::Seconds(0)};
    common::Seconds duration{common::Seconds(0)};
    // Encoded, bounded PNG/JPEG bytes; never a client-local path.
    std::string imageData{};
};

struct RegisterUserCommand {
    std::string name;
    std::string password;
    std::string email{};
};

} // namespace auction::application
