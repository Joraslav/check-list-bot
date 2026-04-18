#include "Bot.hpp"
#include "Log.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {

std::optional<std::string> ReadEnvVar(const char* variable_name) {
    // std::getenv is safe here: single-threaded startup, before any threads are spawned.
    const char* value = std::getenv(variable_name);  // NOLINT(concurrency-mt-unsafe)
    if (value == nullptr) {
        return std::nullopt;
    }

    return std::string(value);
}

int32_t ReadLongPollTimeoutSec() {
    constexpr std::string_view kTimeoutEnvName = "TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC";
    constexpr int32_t kMinTimeoutSec = 1;
    constexpr int32_t kMaxTimeoutSec = 50;

    const auto timeout_env = ReadEnvVar("TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC");
    if (!timeout_env.has_value()) {
        return bot::Bot::kDefaultLongPollTimeoutSec;
    }

    int32_t parsed_timeout = 0;
    const char* begin = timeout_env->data();
    const char* end = begin + timeout_env->size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed_timeout);

    if (ec != std::errc() || ptr != end || parsed_timeout < kMinTimeoutSec ||
        parsed_timeout > kMaxTimeoutSec) {
        std::cerr << "Warning: " << kTimeoutEnvName << " must be integer in range ["
                  << kMinTimeoutSec << ", " << kMaxTimeoutSec << "], fallback to "
                  << bot::Bot::kDefaultLongPollTimeoutSec << "\n";
        return bot::Bot::kDefaultLongPollTimeoutSec;
    }

    return parsed_timeout;
}

bool ReadBoolEnv(const char* variable_name, bool default_value) {
    const auto value = ReadEnvVar(variable_name);
    if (!value.has_value()) {
        return default_value;
    }

    if (*value == "1" || *value == "true" || *value == "TRUE" || *value == "yes" ||
        *value == "YES" || *value == "on" || *value == "ON") {
        return true;
    }

    if (*value == "0" || *value == "false" || *value == "FALSE" || *value == "no" ||
        *value == "NO" || *value == "off" || *value == "OFF") {
        return false;
    }

    std::cerr << "Warning: " << variable_name << " has unsupported value '" << *value
              << "', fallback to " << (default_value ? "true" : "false") << "\n";
    return default_value;
}

int32_t ReadWebhookCleanupAttempts() {
    constexpr const char* kEnvName = "TELEGRAM_BOT_WEBHOOK_CLEANUP_MAX_ATTEMPTS";
    constexpr int32_t kMinAttempts = 1;
    constexpr int32_t kMaxAttempts = 10;

    const auto attempts_env = ReadEnvVar(kEnvName);
    if (!attempts_env.has_value()) {
        return bot::Bot::kDefaultWebhookCleanupMaxAttempts;
    }

    int32_t parsed_attempts = 0;
    const char* begin = attempts_env->data();
    const char* end = begin + attempts_env->size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed_attempts);

    if (ec != std::errc() || ptr != end || parsed_attempts < kMinAttempts ||
        parsed_attempts > kMaxAttempts) {
        std::cerr << "Warning: " << kEnvName << " must be integer in range [" << kMinAttempts
                  << ", " << kMaxAttempts << "], fallback to "
                  << bot::Bot::kDefaultWebhookCleanupMaxAttempts << "\n";
        return bot::Bot::kDefaultWebhookCleanupMaxAttempts;
    }

    return parsed_attempts;
}

bool SetEnvVarIfMissing(const char* variable_name, const std::string& value) {
    if (ReadEnvVar(variable_name).has_value()) {
        return true;
    }

    // setenv is safe here: single-threaded startup, before any threads are spawned.
    if (::setenv(variable_name, value.c_str(), 0) != 0) {  // NOLINT(concurrency-mt-unsafe)
        std::cerr << "Warning: failed to set " << variable_name
                  << " from TELEGRAM_BOT_PROXY, proxy may be ignored\n";
        return false;
    }

    return true;
}

void ConfigureProxyFromEnv() {
    constexpr std::string_view kProxyEnvName = "TELEGRAM_BOT_PROXY";

    const auto proxy_url = ReadEnvVar("TELEGRAM_BOT_PROXY");
    if (!proxy_url.has_value()) {
        return;
    }

    const bool all_proxy_ok = SetEnvVarIfMissing("ALL_PROXY", *proxy_url);
    const bool https_proxy_ok = SetEnvVarIfMissing("HTTPS_PROXY", *proxy_url);

    if (all_proxy_ok && https_proxy_ok) {
        std::cerr << "Info: " << kProxyEnvName
                  << " applied to ALL_PROXY and HTTPS_PROXY for libcurl\n";
    }
}

}  // namespace

int main() {
    try {
        ConfigureProxyFromEnv();

        const std::optional<std::string> token_env = ReadEnvVar("TELEGRAM_BOT_TOKEN");
        if (!token_env.has_value()) {
            std::cerr << "Error: TELEGRAM_BOT_TOKEN environment variable is not set\n";
            return 1;
        }

        const std::string& token = token_env.value();
        const std::string db_path = ReadEnvVar("TELEGRAM_BOT_DB_PATH").value_or("tasks.db");
        const std::string log_directory = ReadEnvVar("TELEGRAM_BOT_LOG_DIR").value_or("out/logs");
        const int32_t long_poll_timeout_sec = ReadLongPollTimeoutSec();
        const bool clear_webhook_on_start =
            ReadBoolEnv("TELEGRAM_BOT_CLEAR_WEBHOOK_ON_START", false);
        const int32_t webhook_cleanup_max_attempts = ReadWebhookCleanupAttempts();

        logging::LogConfig log_config;
        log_config.file_log_directory = log_directory;
        logging::Log::GetInstance().Configure(std::move(log_config));

        bot::Bot my_bot(token, db_path, long_poll_timeout_sec, clear_webhook_on_start,
                        webhook_cleanup_max_attempts);
        my_bot.Run();
    } catch (const bot::Bot::TgException& e) {
        std::cerr << "Telegram error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: unknown exception\n";
        return 1;
    }

    return 0;
}
