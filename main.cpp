#include "Bot.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

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

}  // namespace

int main() {
    try {
        const std::optional<std::string> token_env = ReadEnvVar("TELEGRAM_BOT_TOKEN");
        if (!token_env.has_value()) {
            std::cerr << "Error: TELEGRAM_BOT_TOKEN environment variable is not set\n";
            return 1;
        }

        const std::string& token = token_env.value();
        const std::string db_path = ReadEnvVar("TELEGRAM_BOT_DB_PATH").value_or("tasks.db");
        const int32_t long_poll_timeout_sec = ReadLongPollTimeoutSec();

        bot::Bot my_bot(token, db_path, long_poll_timeout_sec);
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
