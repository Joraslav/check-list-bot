#pragma once

#include "tgbot/tgbot.h"

namespace bot
{

class ReplyKeyboardHandler {
public:
    /**
     * @brief Register reply keyboard button handlers.
     * @param bot Reference to TgBot::Bot
     */
    static void Register(TgBot::Bot& bot);

private:
    static void OnButton(TgBot::Bot& bot, TgBot::Message::Ptr message);
};

} // namespace bot
