#include "ConsoleHandler.hpp"

#include "../Bot.hpp"
#include "../../database/TaskDB.hpp"
#include "../../log/Log.hpp"

#include <format>

DEFINE_LOG_CATEGORY_STATIC(ConsoleHandlerLog);

namespace bot
{

void ConsoleHandler::Register(TgBot::Bot& bot)
{
    // Command handlers
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        OnStart(bot, message);
    });
    bot.getEvents().onCommand("help", [&bot](TgBot::Message::Ptr message) {
        OnHelp(bot, message);
    });
    bot.getEvents().onCommand("list", [&bot](TgBot::Message::Ptr message) {
        OnList(bot, message);
    });
    bot.getEvents().onCommand("add", [&bot](TgBot::Message::Ptr message) {
        OnAdd(bot, message);
    });
    bot.getEvents().onCommand("delete", [&bot](TgBot::Message::Ptr message) {
        OnDelete(bot, message);
    });
    bot.getEvents().onCommand("done", [&bot](TgBot::Message::Ptr message) {
        OnDone(bot, message);
    });

    // Any non-command text
    bot.getEvents().onNonCommandMessage([&bot](TgBot::Message::Ptr message) {
        OnUnknown(bot, message);
    });

    LOG(ConsoleHandlerLog, INFO, "ConsoleHandler registered");
}

void ConsoleHandler::OnStart(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    std::string welcome = "Welcome to Task Bot!\\n"
                          "Available commands:\\n"
                          "/start - Show this message\\n"
                          "/help - Show help\\n"
                          "/list - List tasks\\n"
                          "/add - Add a new task\\n"
                          "/delete - Delete a task\\n"
                          "/done - Mark task as done\\n"
                          "Use the keyboard below to interact.";
    try {
        bot.getApi().sendMessage(message->chat->id, welcome, nullptr, nullptr, ReplyKeyboard::CreateMainMenu());
        LOG(ConsoleHandlerLog, INFO, "Sent start message to chat {}", message->chat->id);
    } catch (const TgBot::TgException& e) {
        LOG(ConsoleHandlerLog, ERROR, "Failed to send start message: {}", e.what());
    }
}

void ConsoleHandler::OnHelp(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    std::string help = "This bot helps you manage tasks.\\n"
                       "Commands:\\n"
                       "/list - show all tasks\\n"
                       "/add <description> - add a task\\n"
                       "/delete <id> - delete a task by ID\\n"
                       "/done <id> - mark task as done\\n"
                       "You can also use inline keyboards for quick actions.";
    try {
        bot.getApi().sendMessage(message->chat->id, help);
    } catch (const TgBot::TgException& e) {
        LOG(ConsoleHandlerLog, ERROR, "Failed to send help: {}", e.what());
    }
}

void ConsoleHandler::OnList(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    // In a real implementation, you would fetch tasks from database
    // For now, we'll send a placeholder
    std::string response = "Your tasks:\\n"
                           "1. Buy groceries (pending)\\n"
                           "2. Write report (done)\\n"
                           "3. Call mom (pending)";
    try {
        auto keyboard = InlineKeyboard::CreateTaskSelection({
            {1, "Buy groceries"},
            {2, "Write report"},
            {3, "Call mom"}
        });
        bot.getApi().sendMessage(message->chat->id, response, nullptr, nullptr, keyboard);
    } catch (const TgBot::TgException& e) {
        LOG(ConsoleHandlerLog, ERROR, "Failed to send list: {}", e.what());
    }
}

void ConsoleHandler::OnAdd(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    std::string text = message->text;
    // Remove "/add " prefix
    size_t pos = text.find(' ');
    if (pos == std::string::npos) {
        bot.getApi().sendMessage(message->chat->id, "Please provide a task description after /add");
        return;
    }
    std::string description = text.substr(pos + 1);
    // In real implementation, insert into database
    std::string reply = std::format("Task added: {}", description);
    bot.getApi().sendMessage(message->chat->id, reply);
}

void ConsoleHandler::OnDelete(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    std::string text = message->text;
    size_t pos = text.find(' ');
    if (pos == std::string::npos) {
        bot.getApi().sendMessage(message->chat->id, "Please provide a task ID after /delete");
        return;
    }
    std::string idStr = text.substr(pos + 1);
    // In real implementation, delete from database
    std::string reply = std::format("Task {} deleted (simulated)", idStr);
    bot.getApi().sendMessage(message->chat->id, reply);
}

void ConsoleHandler::OnDone(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    std::string text = message->text;
    size_t pos = text.find(' ');
    if (pos == std::string::npos) {
        bot.getApi().sendMessage(message->chat->id, "Please provide a task ID after /done");
        return;
    }
    std::string idStr = text.substr(pos + 1);
    std::string reply = std::format("Task {} marked as done (simulated)", idStr);
    bot.getApi().sendMessage(message->chat->id, reply);
}

void ConsoleHandler::OnUnknown(TgBot::Bot& bot, TgBot::Message::Ptr message)
{
    // If it's a text message that is not a command, treat as potential task description
    if (!message->text.empty()) {
        std::string reply = "I didn't understand that. Type /help for commands.";
        bot.getApi().sendMessage(message->chat->id, reply);
    }
}

} // namespace bot