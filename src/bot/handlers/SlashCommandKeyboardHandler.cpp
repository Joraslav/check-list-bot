#include "SlashCommandKeyboardHandler.hpp"

#include "Bot.hpp"
#include "Log.hpp"
#include "magic_enum/magic_enum.hpp"
#include "SlashCommandKeyboard.hpp"

#include <format>
#include <string>

DEFINE_LOG_CATEGORY_STATIC(SlashCommandHandlerLog);

namespace bot {

void SlashCommandKeyboardHandler::Register(TgBot::Bot& bot) {
    // Register all slash command handlers
    bot.getEvents().onCommand(
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::START),
        [&bot](const Message::Ptr& message) { HandleCommand(bot, message, SlashCommand::START); });
    bot.getEvents().onCommand(
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::HELP),
        [&bot](const Message::Ptr& message) { HandleCommand(bot, message, SlashCommand::HELP); });
    bot.getEvents().onCommand(
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::LIST),
        [&bot](const Message::Ptr& message) { HandleCommand(bot, message, SlashCommand::LIST); });
    bot.getEvents().onCommand(
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::ADD),
        [&bot](const Message::Ptr& message) { HandleCommand(bot, message, SlashCommand::ADD); });
    bot.getEvents().onCommand(
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::DELETE),
        [&bot](const Message::Ptr& message) { HandleCommand(bot, message, SlashCommand::DELETE); });
    bot.getEvents().onCommand(
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::DONE),
        [&bot](const Message::Ptr& message) { HandleCommand(bot, message, SlashCommand::DONE); });
    bot.getEvents().onCommand(
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::CANCEL),
        [&bot](const Message::Ptr& message) { HandleCommand(bot, message, SlashCommand::CANCEL); });

    LOG(SlashCommandHandlerLog, INFO, "SlashCommandKeyboardHandler registered with {} commands",
        magic_enum::enum_count<SlashCommand>());
}

void SlashCommandKeyboardHandler::HandleCommand(TgBot::Bot& bot, const Message::Ptr& message,
                                                SlashCommand command) {
    using enum SlashCommand;
    switch (command) {
        case START:
            OnStart(bot, message);
            break;
        case HELP:
            OnHelp(bot, message);
            break;
        case LIST:
            OnList(bot, message);
            break;
        case ADD:
            OnAdd(bot, message);
            break;
        case DELETE:
            OnDelete(bot, message);
            break;
        case DONE:
            OnDone(bot, message);
            break;
        case CANCEL:
            OnCancel(bot, message);
            break;
    }
}

void SlashCommandKeyboardHandler::OnStart(TgBot::Bot& bot, const Message::Ptr& message) {
    const std::string response = std::format(
        "👋 Welcome to Check-List Bot!\n\n"
        "I will help you manage your tasks efficiently.\n\n"
        "Use {} to see all available commands.",
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::HELP));

    try {
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, INFO, "User {} started the bot", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send start message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnHelp(TgBot::Bot& bot, const Message::Ptr& message) {
    try {
        bot.getApi().sendMessage(message->chat->id, SlashCommandKeyboard::GetHelpText());
        LOG(SlashCommandHandlerLog, DEBUG, "Help message sent to user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send help message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnList(TgBot::Bot& bot, const Message::Ptr& message) {
    const std::string response = "📋 Task List\n\nYour tasks will be displayed here.";
    try {
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "List command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send list message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnAdd(TgBot::Bot& bot, const Message::Ptr& message) {
    const std::string response = "➕ Add Task\n\nPlease describe your new task.";
    try {
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "Add command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send add message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnDelete(TgBot::Bot& bot, const Message::Ptr& message) {
    const std::string response = "🗑️ Delete Task\n\nSelect a task to delete.";
    try {
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "Delete command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send delete message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnDone(TgBot::Bot& bot, const Message::Ptr& message) {
    const std::string response = "✅ Mark Task as Done\n\nSelect a task to mark as completed.";
    try {
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "Done command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send done message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnCancel(TgBot::Bot& bot, const Message::Ptr& message) {
    const std::string formatted_response =
        std::format("⛔ Operation cancelled. Type {} for help.",
                    SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::HELP));
    try {
        bot.getApi().sendMessage(message->chat->id, formatted_response);
        LOG(SlashCommandHandlerLog, DEBUG, "Cancel command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send cancel message: {}", e.what());
    }
}

}  // namespace bot
