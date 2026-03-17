#include "Bot.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

namespace {

std::optional<std::string> ReadEnvVar(const char* variable_name) {
    const char* value = std::getenv(variable_name);  // NOLINT(concurrency-mt-unsafe)
    if (value == nullptr) {
        return std::nullopt;
    }

    return std::string(value);
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

        bot::Bot my_bot(token, db_path);
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
