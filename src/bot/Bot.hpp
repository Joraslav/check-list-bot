#pragma once

#include "tgbot/Bot.h"
#include "tgbot/types/Message.h"

#include "TaskDB.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace bot {

class Bot {
 public:
    using Message = TgBot::Message;
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
