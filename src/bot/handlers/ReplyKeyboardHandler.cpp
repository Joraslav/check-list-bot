#include "ReplyKeyboardHandler.hpp"

#include "Log.hpp"
#include "SlashCommandKeyboard.hpp"
#include "SlashCommandKeyboardHandler.hpp"

#include <format>
#include <string>

DEFINE_LOG_CATEGORY_STATIC(ReplyKeyboardHandlerLog);

namespace bot {

void ReplyKeyboardHandler::Register(TgBot::Bot& bot) {
    // Handle messages that contain text matching button labels
    bot.getEvents().onNonCommandMessage(
        [&bot](const Message::Ptr& message) { OnButton(bot, message); });

    LOG(ReplyKeyboardHandlerLog, INFO, "ReplyKeyboardHandler registered");
}

void ReplyKeyboardHandler::OnButton(TgBot::Bot& bot, const Message::Ptr& message) {
    if (message->text.empty()) {
        return;
    }

    if (const auto command = SlashCommandKeyboard::ParseCommand(message->text);
        command.has_value()) {
        SlashCommandKeyboardHandler::HandleCommand(bot, message, *command);
        LOG(ReplyKeyboardHandlerLog, DEBUG, "Reply keyboard command '{}' routed", message->text);
        return;
    }

    const std::string response = std::format("You pressed: {}", message->text);
    try {
        bot.getApi().sendMessage(message->chat->id, response);
    } catch (const TgBot::TgException& e) {
        LOG(ReplyKeyboardHandlerLog, ERROR, "Failed to send button response: {}", e.what());
    }
}

}  // namespace bot