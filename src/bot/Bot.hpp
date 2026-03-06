#pragma once

#include "tgbot/tgbot.h"

#include "TaskDB.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace bot {

class Bot {
 public:
    Bot(std::string_view token);
    ~Bot();

    /// @brief Run the bot (blocking)
    void Run();

 private:
    void SetupHandlers();

    std::unique_ptr<TgBot::Bot> tg_bot_;
    std::unique_ptr<database::TaskDB> db_;
};

}  // namespace bot
