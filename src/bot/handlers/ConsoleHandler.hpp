#pragma once

#include "tgbot/tgbot.h"

#include "../keyboards/ReplyKeyboard.hpp"
#include "../keyboards/InlineKeyboard.hpp"

namespace bot
{

class ConsoleHandler {
public:
    /**
     * @brief Register command handlers for the bot.
     * @param bot Reference to TgBot::Bot
     */
    static void Register(TgBot::Bot& bot);

private:
    static void OnStart(TgBot::Bot& bot, TgBot::Message::Ptr message);
    static void OnHelp(TgBot::Bot& bot, TgBot::Message::Ptr message);
    static void OnList(TgBot::Bot& bot, TgBot::Message::Ptr message);
    static void OnAdd(TgBot::Bot& bot, TgBot::Message::Ptr message);
    static void OnDelete(TgBot::Bot& bot, TgBot::Message::Ptr message);
    static void OnDone(TgBot::Bot& bot, TgBot::Message::Ptr message);
    static void OnUnknown(TgBot::Bot& bot, TgBot::Message::Ptr message);
};

} // namespace bot