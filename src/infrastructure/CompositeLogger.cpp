#include "auction/infrastructure/CompositeLogger.h"

namespace auction::infrastructure {

void CompositeLogger::addLogger(std::shared_ptr<domain::ILogger> logger) {
    if (logger) {
        loggers_.push_back(std::move(logger));
    }
}

void CompositeLogger::debug(std::string_view message) {
    for (const auto& logger : loggers_) {
        logger->debug(message);
    }
}

void CompositeLogger::info(std::string_view message) {
    for (const auto& logger : loggers_) {
        logger->info(message);
    }
}

void CompositeLogger::warn(std::string_view message) {
    for (const auto& logger : loggers_) {
        logger->warn(message);
    }
}

void CompositeLogger::error(std::string_view message) {
    for (const auto& logger : loggers_) {
        logger->error(message);
    }
}

} // namespace auction::infrastructure
