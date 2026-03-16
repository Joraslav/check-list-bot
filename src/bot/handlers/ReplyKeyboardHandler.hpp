#pragma once

#include "tgbot/Bot.h"
#include "tgbot/types/Message.h"

namespace bot {

class ReplyKeyboardHandler {
 public:
    using Message = TgBot::Message;

    /**
     * @brief Register reply keyboard button handlers.
     * @param bot Reference to TgBot::Bot
     */
    static void Register(TgBot::Bot& bot);

 private:
    static void OnButton(TgBot::Bot& bot, const Message::Ptr& message);
};

}  // namespace bot
