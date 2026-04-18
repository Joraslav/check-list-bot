#pragma once

#include "tgbot/Bot.h"
#include "tgbot/TgException.h"
#include "tgbot/types/Message.h"

#include "SlashCommandKeyboard.hpp"

namespace database {
class TaskDB;
}

namespace bot {

class SlashCommandKeyboardHandler final {
 public:
    using Message = TgBot::Message;
    using TgException = TgBot::TgException;

    /**
     * @brief Register slash command handlers.
     * @param bot Reference to TgBot::Bot
     * @param task_db Reference to task database
     */
    static void Register(TgBot::Bot& bot, database::TaskDB& task_db);

    /**
     * @brief Handle command action by enum value.
     * @param bot Reference to TgBot::Bot
     * @param message Incoming message
     * @param command Parsed command
     */
    static void HandleCommand(TgBot::Bot& bot, const Message::Ptr& message, SlashCommand command);

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
