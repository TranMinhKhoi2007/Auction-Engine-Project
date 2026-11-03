#pragma once

#include "auction/application/Commands.h"
#include "auction/domain/entities/SanPhamDauGia.h"
#include <memory>

namespace auction::application {

class ProductFactory {
public:
    [[nodiscard]] static std::unique_ptr<domain::SanPhamDauGia> createProduct(
        common::ProductId id, const CreateProductCommand& command);
};

} // namespace auction::application
