#pragma once

#include "tgbot/tgbot.h"

namespace bot
{

class InlineKeyboardHandler {
public:
    /**
     * @brief Register inline keyboard callback handlers.
     * @param bot Reference to TgBot::Bot
     */
    static void Register(TgBot::Bot& bot);

private:
    static void OnCallback(TgBot::Bot& bot, TgBot::CallbackQuery::Ptr query);
};

} // namespace bot