#include "SlashCommandKeyboardHandler.hpp"

#include "Log.hpp"
#include "magic_enum/magic_enum.hpp"
#include "TaskDB.hpp"

#include <charconv>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

DEFINE_LOG_CATEGORY_STATIC(SlashCommandHandlerLog);

namespace bot {

namespace {

database::TaskDB* g_task_db = nullptr;

bool ValidateMessage(const SlashCommandKeyboardHandler::Message::Ptr& message) {
    return message != nullptr && message->chat != nullptr && message->from != nullptr;
}

std::string BuildTaskListText(const database::TaskList& tasks) {
    if (tasks.empty()) {
        return "📋 You have no tasks yet. Use /add <text> to create one.";
    }

    std::string response = "📋 Your tasks:\n\n";
    for (const auto& task : tasks) {
        const std::string status = (task.status == database::TaskStatus::COMPLETED) ? "✅" : "🟡";
        response += std::format("{} #{} {}\n", status, task.id, task.text);
    }

    response += "\nUse /done <id> or /delete <id> to manage tasks.";
    return response;
}

std::string ExtractCommandArgument(std::string_view raw_text, std::string_view command_with_slash) {
    if (raw_text.size() <= command_with_slash.size()) {
        return {};
    }

    std::string_view tail = raw_text.substr(command_with_slash.size());
    const auto first_non_space = tail.find_first_not_of(" \t");
    if (first_non_space == std::string_view::npos) {
        return {};
    }

    tail.remove_prefix(first_non_space);
    return std::string(tail);
}

std::optional<int64_t> ParseTaskId(std::string_view value) {
    int64_t parsed_id = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed_id);
    if (ec != std::errc() || ptr != end || parsed_id <= 0) {
        return std::nullopt;
    }

    return parsed_id;
}

void EnsureUserExists(database::TaskDB& task_db,
                      const SlashCommandKeyboardHandler::Message::Ptr& message) {
    if (task_db.IsUserExist(message->from->id)) {
        return;
    }

    std::string user_name;
    if (!message->from->username.empty()) {
        user_name = message->from->username;
    } else if (!message->from->firstName.empty()) {
        user_name = message->from->firstName;
    } else {
        user_name = std::format("user_{}", message->from->id);
    }

    task_db.AddUser(message->from->id, user_name);
}

std::optional<database::Task> FindTaskById(database::TaskDB& task_db,
                                           const SlashCommandKeyboardHandler::Message::Ptr& message,
                                           int64_t task_id) {
    const auto tasks = task_db.GetAllTasks(message->from->id);
    if (!tasks.has_value()) {
        return std::nullopt;
    }

    for (const auto& task : tasks.value()) {
        if (task.id == task_id) {
            return task;
        }
    }

    return std::nullopt;
}

void LogIncomingMessage(const SlashCommandKeyboardHandler::Message::Ptr& message) {
    if (message == nullptr || message->chat == nullptr) {
        return;
    }

    const std::string text = message->text.empty() ? "<empty>" : message->text;
    LOG(SlashCommandHandlerLog, INFO, "Incoming message in chat {}: {}", message->chat->id, text);
}

void LogUnknownCommand(const SlashCommandKeyboardHandler::Message::Ptr& message) {
    if (message == nullptr || message->chat == nullptr) {
        return;
    }

    const std::string text = message->text.empty() ? "<empty>" : message->text;
    LOG(SlashCommandHandlerLog, WARNING, "Unknown command received in chat {}: {}",
        message->chat->id, text);
}

void RegisterSlashCommands(TgBot::Bot& bot) {
    // Register all slash command handlers. onCommand expects command name without '/'.
    bot.getEvents().onCommand(SlashCommandKeyboard::GetCommandName(SlashCommand::START),
                              [&bot](const SlashCommandKeyboardHandler::Message::Ptr& message) {
                                  SlashCommandKeyboardHandler::HandleCommand(bot, message,
                                                                             SlashCommand::START);
                              });
    bot.getEvents().onCommand(SlashCommandKeyboard::GetCommandName(SlashCommand::HELP),
                              [&bot](const SlashCommandKeyboardHandler::Message::Ptr& message) {
                                  SlashCommandKeyboardHandler::HandleCommand(bot, message,
                                                                             SlashCommand::HELP);
                              });
    bot.getEvents().onCommand(SlashCommandKeyboard::GetCommandName(SlashCommand::LIST),
                              [&bot](const SlashCommandKeyboardHandler::Message::Ptr& message) {
                                  SlashCommandKeyboardHandler::HandleCommand(bot, message,
                                                                             SlashCommand::LIST);
                              });
    bot.getEvents().onCommand(SlashCommandKeyboard::GetCommandName(SlashCommand::ADD),
                              [&bot](const SlashCommandKeyboardHandler::Message::Ptr& message) {
                                  SlashCommandKeyboardHandler::HandleCommand(bot, message,
                                                                             SlashCommand::ADD);
                              });
    bot.getEvents().onCommand(SlashCommandKeyboard::GetCommandName(SlashCommand::DELETE),
                              [&bot](const SlashCommandKeyboardHandler::Message::Ptr& message) {
                                  SlashCommandKeyboardHandler::HandleCommand(bot, message,
                                                                             SlashCommand::DELETE);
                              });
    bot.getEvents().onCommand(SlashCommandKeyboard::GetCommandName(SlashCommand::DONE),
                              [&bot](const SlashCommandKeyboardHandler::Message::Ptr& message) {
                                  SlashCommandKeyboardHandler::HandleCommand(bot, message,
                                                                             SlashCommand::DONE);
                              });
    bot.getEvents().onCommand(SlashCommandKeyboard::GetCommandName(SlashCommand::CANCEL),
                              [&bot](const SlashCommandKeyboardHandler::Message::Ptr& message) {
                                  SlashCommandKeyboardHandler::HandleCommand(bot, message,
                                                                             SlashCommand::CANCEL);
                              });
}

database::TaskDB& GetTaskDbOrThrow() {
    if (g_task_db == nullptr) {
        throw std::logic_error("Task database is not configured for slash commands");
    }
    return *g_task_db;
}

}  // namespace

void SlashCommandKeyboardHandler::Register(TgBot::Bot& bot, database::TaskDB& task_db) {
    g_task_db = &task_db;

    bot.getEvents().onAnyMessage(LogIncomingMessage);
    bot.getEvents().onUnknownCommand(LogUnknownCommand);
    RegisterSlashCommands(bot);

    LOG(SlashCommandHandlerLog, INFO, "SlashCommandKeyboardHandler registered with {} commands",
        magic_enum::enum_count<SlashCommand>());
}

void SlashCommandKeyboardHandler::HandleCommand(TgBot::Bot& bot, const Message::Ptr& message,
                                                SlashCommand command) {
    if (!ValidateMessage(message)) {
        LOG(SlashCommandHandlerLog, WARNING,
            "Received command {:?} with incomplete message payload",
            magic_enum::enum_name(command));
        return;
    }

    LOG(SlashCommandHandlerLog, INFO, "Command '{}' received from chat {}",
        SlashCommandKeyboard::GetCommandWithSlash(command), message->chat->id);

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
    if (!ValidateMessage(message)) {
        return;
    }

    const std::string response = std::format(
        "👋 Welcome to Check-List Bot!\n\n"
        "I will help you manage your tasks efficiently.\n\n"
        "Use {} to see all available commands.\n"
        "Quick start: /add Buy milk",
        SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::HELP));

    try {
        EnsureUserExists(GetTaskDbOrThrow(), message);
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, INFO, "User {} started the bot", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send start message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnHelp(TgBot::Bot& bot, const Message::Ptr& message) {
    if (!ValidateMessage(message)) {
        return;
    }

    const std::string extra_help =
        "\nExamples:\n"
        "/add Buy groceries\n"
        "/list\n"
        "/done 3\n"
        "/delete 2\n";

    try {
        bot.getApi().sendMessage(message->chat->id,
                                 SlashCommandKeyboard::GetHelpText() + extra_help);
        LOG(SlashCommandHandlerLog, DEBUG, "Help message sent to user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send help message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnList(TgBot::Bot& bot, const Message::Ptr& message) {
    if (!ValidateMessage(message)) {
        return;
    }

    try {
        auto& task_db = GetTaskDbOrThrow();
        EnsureUserExists(task_db, message);
        const auto tasks = task_db.GetAllTasks(message->from->id);
        const std::string response =
            tasks.has_value() ? BuildTaskListText(*tasks) : BuildTaskListText(database::TaskList{});
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "List command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send list message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnAdd(TgBot::Bot& bot, const Message::Ptr& message) {
    if (!ValidateMessage(message)) {
        return;
    }

    try {
        auto& task_db = GetTaskDbOrThrow();
        EnsureUserExists(task_db, message);

        const std::string task_text = ExtractCommandArgument(
            message->text, SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::ADD));
        if (task_text.empty()) {
            bot.getApi().sendMessage(message->chat->id,
                                     "Usage: /add <task text>\nExample: /add Buy groceries");
            return;
        }

        const auto task_id =
            task_db.AddTask(message->from->id, task_text, database::TaskStatus::ACTIVE);
        if (!task_id.has_value()) {
            bot.getApi().sendMessage(message->chat->id, "Failed to create task. Please try again.");
            return;
        }

        const std::string response = std::format("➕ Task added: #{} {}", *task_id, task_text);
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "Add command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send add message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnDelete(TgBot::Bot& bot, const Message::Ptr& message) {
    if (!ValidateMessage(message)) {
        return;
    }

    try {
        auto& task_db = GetTaskDbOrThrow();
        EnsureUserExists(task_db, message);

        const std::string task_id_arg = ExtractCommandArgument(
            message->text, SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::DELETE));
        const auto task_id = ParseTaskId(task_id_arg);
        if (!task_id.has_value()) {
            bot.getApi().sendMessage(message->chat->id,
                                     "Usage: /delete <task_id>\nExample: /delete 2");
            return;
        }

        const auto task = FindTaskById(task_db, message, *task_id);
        if (!task.has_value()) {
            bot.getApi().sendMessage(message->chat->id,
                                     std::format("Task #{} not found.", *task_id));
            return;
        }

        task_db.DeleteTask(message->from->id, *task_id);
        const std::string response = std::format("🗑️ Task #{} deleted.", *task_id);
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "Delete command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send delete message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnDone(TgBot::Bot& bot, const Message::Ptr& message) {
    if (!ValidateMessage(message)) {
        return;
    }

    try {
        auto& task_db = GetTaskDbOrThrow();
        EnsureUserExists(task_db, message);

        const std::string task_id_arg = ExtractCommandArgument(
            message->text, SlashCommandKeyboard::GetCommandWithSlash(SlashCommand::DONE));
        const auto task_id = ParseTaskId(task_id_arg);
        if (!task_id.has_value()) {
            bot.getApi().sendMessage(message->chat->id, "Usage: /done <task_id>\nExample: /done 3");
            return;
        }

        const auto task = FindTaskById(task_db, message, *task_id);
        if (!task.has_value()) {
            bot.getApi().sendMessage(message->chat->id,
                                     std::format("Task #{} not found.", *task_id));
            return;
        }

        if (task->status == database::TaskStatus::COMPLETED) {
            bot.getApi().sendMessage(message->chat->id,
                                     std::format("Task #{} is already completed.", *task_id));
            return;
        }

        task_db.UpdateTaskStatus(message->from->id, *task_id, database::TaskStatus::COMPLETED);
        const std::string response = std::format("✅ Task #{} marked as completed.", *task_id);
        bot.getApi().sendMessage(message->chat->id, response);
        LOG(SlashCommandHandlerLog, DEBUG, "Done command executed by user {}", message->from->id);
    } catch (const TgException& e) {
        LOG(SlashCommandHandlerLog, ERROR, "Failed to send done message: {}", e.what());
    }
}

void SlashCommandKeyboardHandler::OnCancel(TgBot::Bot& bot, const Message::Ptr& message) {
    if (!ValidateMessage(message)) {
        return;
    }

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
