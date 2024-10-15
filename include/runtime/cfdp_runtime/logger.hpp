#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <mutex>
#include <thread>

namespace cfdp::runtime::logging
{
enum class LogLevel : uint8_t
{
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
};

#if defined(LOG_LEVEL_DEBUG)
constexpr LogLevel log_level_cutoff = LogLevel::Debug;
#elif defined(LOG_LEVEL_INFO)
constexpr LogLevel log_level_cutoff = LogLevel::Info;
#elif defined(LOG_LEVEL_WARN)
constexpr LogLevel log_level_cutoff = LogLevel::Warn;
#elif defined(LOG_LEVEL_ERROR)
constexpr LogLevel log_level_cutoff = LogLevel::Error;
#else
constexpr LogLevel log_level_cutoff = LogLevel::Trace;
#endif

class Logger
{
  public:
    void log(LogLevel level, std::string msg) const noexcept;

  private:
    // NOTE: 14.10.2024 <@uncommon-nickname>
    // We need this mutex to be static, our entrypoint logging functions
    // use templates and each specialization has its own static logger
    // object. This way we will avoid log races.
    static inline std::mutex mutex{};

    [[nodiscard]] inline std::thread::id getThreadID() const noexcept
    {
        return std::this_thread::get_id();
    }

    [[nodiscard]] inline std::string getCurrentTimestamp() const noexcept
    {
        const auto now = std::chrono::system_clock::now();
        return std::format("{:%d-%m-%Y %H:%M:%OS}", now);
    }
};

template <class... Args>
void log(LogLevel level, std::format_string<Args...> msg, Args&&... args) noexcept
{
    if (level < log_level_cutoff)
    {
        return;
    }

    auto log = std::vformat(msg.get(), std::make_format_args(args...));

    static auto logger = Logger{};
    logger.log(level, log);
}

template <class... Args>
void trace(std::format_string<Args...> msg, Args&&... args) noexcept
{
    log(LogLevel::Trace, msg, std::forward<Args>(args)...);
}

template <class... Args>
void debug(std::format_string<Args...> msg, Args&&... args) noexcept
{
    log(LogLevel::Debug, msg, std::forward<Args>(args)...);
}

template <class... Args>
void info(std::format_string<Args...> msg, Args&&... args) noexcept
{
    log(LogLevel::Info, msg, std::forward<Args>(args)...);
}

template <class... Args>
void warn(std::format_string<Args...> msg, Args&&... args) noexcept
{
    log(LogLevel::Warn, msg, std::forward<Args>(args)...);
}

template <class... Args>
void error(std::format_string<Args...> msg, Args&&... args) noexcept
{
    log(LogLevel::Error, msg, std::forward<Args>(args)...);
}
} // namespace cfdp::runtime::logging
