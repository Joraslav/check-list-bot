#pragma once

#include "tgbot/Bot.h"
#include "tgbot/types/Message.h"

namespace bot {

class SlashCommandKeyboardHandler final {
 public:
    using Message = TgBot::Message;
    using TgException = TgBot::TgException;

    /**
     * @brief Register slash command handlers.
     * @param bot Reference to TgBot::Bot
     */
    static void Register(TgBot::Bot& bot);

 private:
    /**
     * @brief Handle /start command.
     */
    static void OnStart(TgBot::Bot& bot, const Message::Ptr& message);

    /**
     * @brief Handle /help command.
     */
    static void OnHelp(TgBot::Bot& bot, const Message::Ptr& message);

    /**
     * @brief Handle /list command.
     */
    static void OnList(TgBot::Bot& bot, const Message::Ptr& message);

    /**
     * @brief Handle /add command.
     */
    static void OnAdd(TgBot::Bot& bot, const Message::Ptr& message);

    /**
     * @brief Handle /delete command.
     */
    static void OnDelete(TgBot::Bot& bot, const Message::Ptr& message);

    /**
     * @brief Handle /done command.
     */
    static void OnDone(TgBot::Bot& bot, const Message::Ptr& message);

    /**
     * @brief Handle /cancel command.
     */
    static void OnCancel(TgBot::Bot& bot, const Message::Ptr& message);
};

}  // namespace bot
