#pragma once
#include "auction/domain/ports/ILogger.h"
#include <iostream>
namespace auction::infrastructure {
class ConsoleLogger final : public domain::ILogger {
public:
    void debug(std::string_view s) override { log("DEBUG",s); }
    void info(std::string_view s) override { log("INFO",s); }
    void warn(std::string_view s) override { log("WARN",s); }
    void error(std::string_view s) override { log("ERROR",s); }
private:
    static void log(std::string_view level,std::string_view s) { std::clog << level << " | " << s << std::endl; }
};
}
