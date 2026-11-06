#pragma once
#include "auction/domain/exceptions/AuctionException.h"
namespace auction::domain {
class AuthorizationException final : public ValidationException {
public:
    using ValidationException::ValidationException;
};
}
