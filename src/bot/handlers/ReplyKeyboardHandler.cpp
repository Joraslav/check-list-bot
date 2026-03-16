#include "ReplyKeyboardHandler.hpp"

#include "Bot.hpp"
#include "Log.hpp"

#include <format>
#include <string>
#include <string_view>

DEFINE_LOG_CATEGORY_STATIC(ReplyKeyboardHandlerLog);

namespace bot {

using std::string_view_literals::operator""sv;

void ReplyKeyboardHandler::Register(TgBot::Bot& bot) {
    // Handle messages that contain text matching button labels
    bot.getEvents().onNonCommandMessage(
        [&bot](const Message::Ptr& message) { OnButton(bot, message); });

    LOG(ReplyKeyboardHandlerLog, INFO, "ReplyKeyboardHandler registered");
}

void ReplyKeyboardHandler::OnButton(TgBot::Bot& bot, const Message::Ptr& message) {
    if (!message->text.empty()) {
        const std::string_view text = message->text;
        // Check if it's a button from our reply keyboard
        if (text == "start"sv || text == "help"sv || text == "list"sv || text == "add"sv ||
            text == "delete"sv || text == "done"sv) {
            // These are already handled by command handlers, ignore
            return;
        }
        // Handle custom button presses
        const std::string response = std::format("You pressed: {}", text);
        try {
            bot.getApi().sendMessage(message->chat->id, response);
        } catch (const TgBot::TgException& e) {
            LOG(ReplyKeyboardHandlerLog, ERROR, "Failed to send button response: {}", e.what());
        }
    }
}

}  // namespace bot