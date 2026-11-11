#pragma once

#include "auction/domain/ports/ILogger.h"
#include <fstream>
#include <mutex>
#include <string>

namespace auction::infrastructure {

class FileLogger final : public domain::ILogger {
public:
    explicit FileLogger(const std::string& filePath);
    ~FileLogger() override;

    void debug(std::string_view message) override;
    void info(std::string_view message) override;
    void warn(std::string_view message) override;
    void error(std::string_view message) override;

private:
    void log(std::string_view level, std::string_view message);

    std::ofstream fileStream_;
    std::mutex mutex_;
};

} // namespace auction::infrastructure
