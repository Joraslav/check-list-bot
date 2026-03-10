#include "Log.hpp"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <string_view>
#include <unordered_map>

namespace logging {

namespace fs = std::filesystem;
namespace ch = std::chrono;

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

// -------------------- Определение констант -------------------- //

/**
 * @brief Map LogLevel to spdlog::level::level_enum
 */
const std::unordered_map<LogLevel, spdlog::level::level_enum> kLevelMap = {
    {LogLevel::INFO, spdlog::level::info},
    {LogLevel::DEBUG, spdlog::level::debug},
    {LogLevel::WARNING, spdlog::level::warn},
    {LogLevel::ERROR, spdlog::level::err},
    {LogLevel::FATAL, spdlog::level::critical}};

/**
 * @brief Path to log directory
 */
const fs::path kLogDirectory = fs::path("../out/logs"sv);

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

// -------------------- Реализация -------------------- //

std::string LogCategory::GetName() const { return name_; }

/**
 * @brief Implementation of Log class
 */
class Log::Impl {
 public:
    Impl() {
        using namespace spdlog;
        const auto console_sink = std::make_shared<sinks::stdout_color_sink_mt>();
        console_logger_ = std::make_unique<logger>("CONSOLE", console_sink);
        console_logger_->set_pattern(kLogPattern);

        const auto file_sink =
            std::make_shared<sinks::basic_file_sink_mt>(MakeLogFile().string(), true);
        file_logger_ = std::make_unique<logger>("FILE", file_sink);
        file_logger_->set_pattern(kLogPattern);
    }

    void Log(LogLevel level, std::string_view message) const {
        const spdlog::level::level_enum spd_level = kLevelMap.at(level);
        if (console_logger_->should_log(spd_level)) {
            console_logger_->log(spd_level, message);
        }
        if (file_logger_->should_log(spd_level)) {
            file_logger_->log(spd_level, message);
        }
        if (level == LogLevel::FATAL) {
            // TODO: Отправка сигнала на остановку работы
        }
    }

 private:
    std::unique_ptr<spdlog::logger> console_logger_;
    std::unique_ptr<spdlog::logger> file_logger_;

    fs::path MakeLogFile() const {
        fs::create_directories(kLogDirectory);
        const auto now = ch::system_clock::now();
        const auto now_seconds = ch::floor<ch::seconds>(now);
        const std::string timestamp = std::format(kTimeStampFormat, now_seconds);
        const std::string log_name = std::format("{}_{}.{}", kLogPrefix, timestamp, kLogExtension);
        return kLogDirectory / log_name;
    }
};

Log::Log() : impl_(std::make_unique<Log::Impl>()) {}
Log::~Log() = default;

void Log::Loging(const LogCategory& category, LogLevel level, std::string_view message) const {
    impl_->Log(level, std::format("[{}] {}", category.GetName(), message));
}

}  // namespace logging
