#pragma once

#include "tgbot/Bot.h"
#include "tgbot/TgException.h"
#include "tgbot/types/CallbackQuery.h"

namespace bot {

class InlineKeyboardHandler final {
 public:
    using CallbackQuery = TgBot::CallbackQuery;
    using TgException = TgBot::TgException;

    /**
     * @brief Register inline keyboard callback handlers.
     * @param bot Reference to TgBot::Bot
     */
    static void Register(TgBot::Bot& bot);

 private:
    static void OnCallback(TgBot::Bot& bot, const CallbackQuery::Ptr& query);
};

}  // namespace bot