#include "auction/infrastructure/FileLogger.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace auction::infrastructure {

namespace {
std::string formatIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &nowTimeT);
#else
    localtime_r(&nowTimeT, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}
} // namespace

FileLogger::FileLogger(const std::string& filePath) {
    std::filesystem::path path(filePath);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    fileStream_.open(filePath, std::ios::out | std::ios::app);
}

FileLogger::~FileLogger() {
    if (fileStream_.is_open()) {
        fileStream_.flush();
        fileStream_.close();
    }
}

void FileLogger::debug(std::string_view message) {
    log("DEBUG", message);
}

void FileLogger::info(std::string_view message) {
    log("INFO ", message);
}

void FileLogger::warn(std::string_view message) {
    log("WARN ", message);
}

void FileLogger::error(std::string_view message) {
    log("ERROR", message);
}

void FileLogger::log(std::string_view level, std::string_view message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!fileStream_.is_open()) {
        return;
    }
    fileStream_ << formatIsoTimestamp() << " [" << level << "] " << message << "\n";
    fileStream_.flush();
}

} // namespace auction::infrastructure
