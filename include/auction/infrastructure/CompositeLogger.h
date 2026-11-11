#pragma once

#include "auction/domain/ports/ILogger.h"
#include <memory>
#include <vector>

namespace auction::infrastructure {

class CompositeLogger final : public domain::ILogger {
public:
    CompositeLogger() = default;

    void addLogger(std::shared_ptr<domain::ILogger> logger);

    void debug(std::string_view message) override;
    void info(std::string_view message) override;
    void warn(std::string_view message) override;
    void error(std::string_view message) override;

private:
    std::vector<std::shared_ptr<domain::ILogger>> loggers_;
};

} // namespace auction::infrastructure
