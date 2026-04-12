#include "Log.hpp"

#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <memory>
#include <string_view>
#include <utility>

namespace logging {

namespace fs = std::filesystem;
namespace ch = std::chrono;

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

// -------------------- Определение констант -------------------- //

/**
 * @brief Pattern for timestamp format
 */
constexpr std::string_view kTimeStampFormat = "{:%d-%m-%Y_%H-%M-%S}"sv;

/**
 * @brief Prefix log file
 */
constexpr std::string_view kLogPrefix = "BotLog"sv;

/**
 * @brief Extension log file
 */
constexpr std::string_view kLogExtension = "log"sv;

/**
 * @brief Pattern for log time message
 */
const std::string kLogPattern = "[%H:%M:%S.%e] [%^%l%$] %v"s;

namespace {

[[nodiscard]] constexpr spdlog::level::level_enum ToSpdlogLevel(const LogLevel level) {
    switch (level) {
        case LogLevel::INFO:
            return spdlog::level::info;
        case LogLevel::DEBUG:
            return spdlog::level::debug;
        case LogLevel::WARNING:
            return spdlog::level::warn;
        case LogLevel::ERROR:
            return spdlog::level::err;
        case LogLevel::FATAL:
            return spdlog::level::critical;
    }

    return spdlog::level::info;
}

}  // namespace

// -------------------- Реализация -------------------- //

std::string LogCategory::GetName() const { return name_; }

/**
 * @brief Implementation of Log class
 */
class Log::Impl {
 public:
    explicit Impl(LogConfig config) : config_(std::move(config)) {
        using namespace spdlog;

        if (config_.enable_console_logging) {
            const auto console_sink = std::make_shared<sinks::stdout_color_sink_mt>();
            console_logger_ = std::make_unique<logger>("CONSOLE", console_sink);
            console_logger_->set_pattern(kLogPattern);
        }

        if (config_.enable_file_logging) {
            const auto file_sink = std::make_shared<sinks::basic_file_sink_mt>(
                MakeLogFile(config_.file_log_directory).string(), true);
            file_logger_ = std::make_unique<logger>("FILE", file_sink);
            file_logger_->set_pattern(kLogPattern);
        }
    }

    void Log(LogLevel level, std::string_view message) const {
        const spdlog::level::level_enum spd_level = ToSpdlogLevel(level);

        if (console_logger_ != nullptr && console_logger_->should_log(spd_level)) {
            console_logger_->log(spd_level, message);
        }

        if (file_logger_ != nullptr && file_logger_->should_log(spd_level)) {
            file_logger_->log(spd_level, message);
        }

        if (level == LogLevel::FATAL) {
            // TODO: Отправка сигнала на остановку работы
        }
    }

 private:
    LogConfig config_;
    std::unique_ptr<spdlog::logger> console_logger_;
    std::unique_ptr<spdlog::logger> file_logger_;

    static fs::path MakeLogFile(const fs::path& log_directory) {
        fs::create_directories(log_directory);
        const auto now = ch::system_clock::now();
        const auto now_seconds = ch::floor<ch::seconds>(now);
        const std::string timestamp = std::format(kTimeStampFormat, now_seconds);
        const std::string log_name = std::format("{}_{}.{}", kLogPrefix, timestamp, kLogExtension);
        return log_directory / log_name;
    }
};

Log::Log() : impl_(std::make_unique<Log::Impl>(LogConfig{})) {}
Log::~Log() = default;

void Log::Configure(LogConfig config) { impl_ = std::make_unique<Log::Impl>(std::move(config)); }

void Log::Loging(const LogCategory& category, LogLevel level, std::string_view message) const {
    impl_->Log(level, std::format("[{}] {}", category.GetName(), message));
}

}  // namespace logging
