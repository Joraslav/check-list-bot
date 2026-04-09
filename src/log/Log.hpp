#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace logging {

enum class LogLevel { INFO, DEBUG, WARNING, ERROR, FATAL };

/**
 * @brief Minimum log level
 */
constexpr LogLevel kMinLevel = LogLevel::INFO;

/**
 * @brief Maximum log level
 */
constexpr LogLevel kMaxLevel = LogLevel::FATAL;

/**
 * @brief Log category class
 */
class LogCategory {
 public:
    explicit LogCategory(std::string name) : name_(std::move(name)) {}
    ~LogCategory() = default;

    /**
     * @brief Get the name of the log category
     * @return `std::string` - name of the log category
     */
    std::string GetName() const;

 private:
    const std::string name_;
};

/**
 * @brief Log class
 * @note Use PImpl
 */
class Log final {
 public:
    static Log& GetInstance() {
        static Log instance;
        return instance;
    }
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(Log&&) = delete;

    void Loging(const LogCategory& category, LogLevel level, std::string_view message) const;

 private:
    Log();
    ~Log();
    class Impl;
    std::unique_ptr<Impl> impl_;
};

template <typename T>
concept ValidLogCategory = std::constructible_from<LogCategory, T>;

template <typename T>
concept LoggableMessage =
    std::convertible_to<T, std::string> || std::convertible_to<T, std::string_view>;

template <LogLevel L>
concept ValidLogLevel = L == LogLevel::INFO || L == LogLevel::DEBUG || L == LogLevel::WARNING ||
                        L == LogLevel::ERROR || L == LogLevel::FATAL;

}  // namespace logging

/**
 * @brief Define log category static
 * @param log_name name of the log category
 */
#define DEFINE_LOG_CATEGORY_STATIC(log_name) const logging::LogCategory log_name(#log_name);

/**
 * @brief Log implementation
 * @param category_name log category
 * @param level log level
 * @param format_str format string
 */
#define LOG_IMPL(category_name, level, format_str, ...)                                          \
    do {                                                                                         \
        if constexpr (logging::LogLevel::level >= logging::kMinLevel &&                          \
                      logging::LogLevel::level <= logging::kMaxLevel)                            \
            static_assert(logging::ValidLogLevel<logging::LogLevel::level>,                      \
                          "Verbosity must be one of: INFO, DEBUG, WARNING, ERROR, FATAL");       \
        static_assert(logging::ValidLogCategory<decltype(category_name)>,                        \
                      "Category must be of type LogCategory");                                   \
        static_assert(logging::LoggableMessage<decltype(format_str)>,                            \
                      "Message must be convertible to std::string or std::string_view");         \
        logging::Log::GetInstance().Loging(category_name, logging::LogLevel::level,              \
                                           std::format(format_str __VA_OPT__(, )##__VA_ARGS__)); \
    } while (false)

#define LOG(category_name, level, format_str, ...) \
    LOG_IMPL(category_name, level, format_str, ##__VA_ARGS__)
