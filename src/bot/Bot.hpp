#pragma once

#include "tgbot/Bot.h"
#include "tgbot/net/HttpClient.h"
#include "tgbot/TgException.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace database {
class TaskDB;
}

namespace bot {

class Bot {
 public:
    using TgException = TgBot::TgException;

    static constexpr int32_t kDefaultLongPollTimeoutSec = 1;
    static constexpr int32_t kDefaultWebhookCleanupMaxAttempts = 3;

    Bot(std::string_view token, const std::filesystem::path& db_path,
        int32_t long_poll_timeout_sec = kDefaultLongPollTimeoutSec,
        bool clear_webhook_on_start = false,
        int32_t webhook_cleanup_max_attempts = kDefaultWebhookCleanupMaxAttempts);
    ~Bot();

    /**
     * @brief Run the bot
     */
    void Run();

 private:
    /**
     * @brief Setup all handlers
     */
    void SetupHandlers();

    std::unique_ptr<TgBot::HttpClient> http_client_;
    std::unique_ptr<TgBot::Bot> tg_bot_;
    std::unique_ptr<database::TaskDB> db_;
    int32_t long_poll_timeout_sec_ = kDefaultLongPollTimeoutSec;
    bool clear_webhook_on_start_ = false;
    int32_t webhook_cleanup_max_attempts_ = kDefaultWebhookCleanupMaxAttempts;
};

}  // namespace bot
