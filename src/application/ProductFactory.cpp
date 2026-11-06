#include "auction/application/ProductFactory.h"
#include "auction/domain/entities/DoCo.h"
#include "auction/domain/entities/ThietBiCongNghe.h"
#include "auction/domain/entities/TranhNgheThuat.h"
#include "auction/domain/exceptions/AuctionException.h"

namespace auction::application {

std::unique_ptr<domain::SanPhamDauGia> ProductFactory::createProduct(
    common::ProductId id, const CreateProductCommand& command) {
    if (command.name.empty()) {
        throw domain::ValidationException("Tên sản phẩm không được để trống");
    }

    switch (command.type) {
    case ProductType::DoCo:
        return std::make_unique<domain::DoCo>(
            id, command.name, command.description, command.sellerId,
            command.eraCentury, command.origin, command.preservationState, command.isAppraised);

    case ProductType::TranhNgheThuat:
        return std::make_unique<domain::TranhNgheThuat>(
            id, command.name, command.description, command.sellerId,
            command.artist, command.yearCreated, command.medium, command.dimensions,
            command.hasCertificate);

    case ProductType::ThietBiCongNghe:
        return std::make_unique<domain::ThietBiCongNghe>(
            id, command.name, command.description, command.sellerId,
            command.brand, command.model, command.warrantyMonths, command.isNew);
    }

    throw domain::ValidationException("Loại sản phẩm không hợp lệ");
}

} // namespace auction::application
