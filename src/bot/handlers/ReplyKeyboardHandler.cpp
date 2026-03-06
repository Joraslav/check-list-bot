#include "ReplyKeyboardHandler.hpp"

#include "../Bot.hpp"
#include "../../log/Log.hpp"

DEFINE_LOG_CATEGORY_STATIC(ReplyKeyboardHandlerLog);

namespace bot
{

void ReplyKeyboardHandler::Register(TgBot::Bot& bot)
{
    // Handle messages that contain text matching button labels
    bot.getEvents().onNonCommandMessage([&bot](TgBot::Message::Ptr message) {
        OnButton(bot, message);
    });

    LOG(ReplyKeyboardHandlerLog, INFO, "ReplyKeyboardHandler registered");
}

void ReplyKeyboardHandler::OnButton(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    if (!message->text.empty()) {
        std::string text = message->text;
        // Check if it's a button from our reply keyboard
        if (text == "/start" || text == "/help" || text == "/list" ||
            text == "/add" || text == "/delete" || text == "/done") {
            // These are already handled by command handlers, ignore
            return;
        }
        // Handle custom button presses
        std::string response = "You pressed: " + text;
        try {
            bot.getApi().sendMessage(message->chat->id, response);
        } catch (const TgBot::TgException& e) {
            LOG(ReplyKeyboardHandlerLog, ERROR, "Failed to send button response: {}", e.what());
        }
    }
}

} // namespace bot