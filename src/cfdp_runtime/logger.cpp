#include <cfdp_runtime/logger.hpp>

#include <iostream>
#include <unordered_map>

namespace
{
using ::cfdp::runtime::logging::LogLevel;

const std::unordered_map<LogLevel, std::string> coloredLevelNames{
    {LogLevel::Trace, "\x1b[37;1m[TRACE]\x1b[0m"},
    {LogLevel::Debug, "\x1b[34;1m[DEBUG]\x1b[0m"},
    { LogLevel::Info, "\x1b[32;1m[INFO]\x1b[0m "},
    { LogLevel::Warn, "\x1b[33;1m[WARN]\x1b[0m "},
    {LogLevel::Error, "\x1b[31;1m[ERROR]\x1b[0m"},
};
} // namespace

void cfdp::runtime::logging::Logger::log(LogLevel level, std::string msg) const noexcept
{
    auto threadID  = getThreadID();
    auto timestamp = getCurrentTimestamp();
    auto levelName = coloredLevelNames.at(level);

    auto finalMsg = std::format("{}  {}  ThreadID({}): {}", std::move(timestamp),
                                std::move(levelName), threadID, std::move(msg));

    {
        std::scoped_lock<std::mutex> lock{mutex};
        std::cout << finalMsg << std::endl;
    }
}
