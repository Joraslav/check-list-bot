#pragma once

#include "tgbot/Bot.h"
#include "tgbot/TgException.h"

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

    Bot(std::string_view token, const std::filesystem::path& db_path);
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

    std::unique_ptr<TgBot::Bot> tg_bot_;
    std::unique_ptr<database::TaskDB> db_;
};

}  // namespace bot
