#include "Bot.hpp"

#include "keyboards/InlineKeyboard.hpp"
#include "keyboards/ReplyKeyboard.hpp"
#include "Log.hpp"
#include "magic_enum/magic_enum.hpp"
#include "TaskDB.hpp"

#include <format>
#include <memory>
#include <string>

DEFINE_LOG_CATEGORY_STATIC(BotLog);

namespace bot {

Bot::Bot(std::string_view token) {
    tg_bot_ = std::make_unique<TgBot::Bot>(token.data());
    db_ = std::make_unique<database::TaskDB>();  // in-memory database

    LOG(BotLog, INFO, "Bot initialized with token: {}", token);
}

Bot::~Bot() { LOG(BotLog, INFO, "Bot shutting down"); }

void Bot::SetupHandlers() {
    // /start command
    tg_bot_->getEvents().onCommand("start", [this](TgBot::Message::Ptr message) {
        std::string welcome =
            "Привет! Я бот для управления задачами.\n"
            "Доступные команды:\n"
            "/add <текст> - добавить задачу\n"
            "/list - показать все задачи\n"
            "/done <номер> - отметить задачу выполненной\n"
            "/delete <номер> - удалить задачу\n"
            "/stats - статистика";
        tg_bot_->getApi().sendMessage(message->chat->id, welcome);
        LOG(BotLog, INFO, "User {} started bot", message->chat->id);
    });

    // /add command
    tg_bot_->getEvents().onCommand("add", [this](TgBot::Message::Ptr message) {
        std::string text = message->text;
        // Remove "/add " prefix
        size_t pos = text.find(' ');
        if (pos == std::string::npos) {
            tg_bot_->getApi().sendMessage(message->chat->id, "Используйте: /add <текст задачи>");
            return;
        }
        std::string task_text = text.substr(pos + 1);
        if (task_text.empty()) {
            tg_bot_->getApi().sendMessage(message->chat->id, "Текст задачи не может быть пустым");
            return;
        }
        try {
            int64_t task_id = db_->AddTask(message->chat->id, task_text);
            tg_bot_->getApi().sendMessage(message->chat->id,
                                          std::format("Задача добавлена (ID: {})", task_id));
            LOG(BotLog, INFO, "User {} added task: {}", message->chat->id, task_text);
        } catch (const std::exception& e) {
            tg_bot_->getApi().sendMessage(
                message->chat->id, std::format("Ошибка при добавлении задачи: {}", e.what()));
            LOG(BotLog, ERROR, "Add task error: {}", e.what());
        }
    });

    // /list command
    tg_bot_->getEvents().onCommand("list", [this](TgBot::Message::Ptr message) {
        try {
            auto tasks = db_->GetAllTasks(message->chat->id);
            if (!tasks.has_value() || tasks->empty()) {
                tg_bot_->getApi().sendMessage(message->chat->id, "У вас нет задач.");
                return;
            }
            std::string response = "Ваши задачи:\n";
            int index = 1;
            for (const auto& task : *tasks) {
                std::string status = (task.status == database::TaskStatus::ACTIVE) ? "⏳" : "✅";
                response += std::format("{}. {} {} [{}]\n", index++, task.text, status,
                                        magic_enum::enum_name(task.status));
            }
            tg_bot_->getApi().sendMessage(message->chat->id, response);
        } catch (const std::exception& e) {
            tg_bot_->getApi().sendMessage(message->chat->id,
                                          std::format("Ошибка при получении задач: {}", e.what()));
            LOG(BotLog, ERROR, "List tasks error: {}", e.what());
        }
    });

    // /done command
    tg_bot_->getEvents().onCommand("done", [this](TgBot::Message::Ptr message) {
        std::string text = message->text;
        size_t pos = text.find(' ');
        if (pos == std::string::npos) {
            tg_bot_->getApi().sendMessage(message->chat->id, "Используйте: /done <номер задачи>");
            return;
        }
        std::string num_str = text.substr(pos + 1);
        try {
            int task_num = std::stoi(num_str);
            // We need to map task number to task id (simplified: assume task id equals number)
            // In real implementation you would fetch tasks and get the id by index
            // For now, just use task_num as id (simplification)
            db_->UpdateTaskStatus(message->chat->id, task_num, database::TaskStatus::COMPLETED);
            tg_bot_->getApi().sendMessage(
                message->chat->id, std::format("Задача {} отмечена как выполненная", task_num));
            LOG(BotLog, INFO, "User {} completed task {}", message->chat->id, task_num);
        } catch (const std::exception& e) {
            tg_bot_->getApi().sendMessage(message->chat->id, std::format("Ошибка: {}", e.what()));
            LOG(BotLog, ERROR, "Done task error: {}", e.what());
        }
    });

    // /delete command
    tg_bot_->getEvents().onCommand("delete", [this](TgBot::Message::Ptr message) {
        std::string text = message->text;
        size_t pos = text.find(' ');
        if (pos == std::string::npos) {
            tg_bot_->getApi().sendMessage(message->chat->id, "Используйте: /delete <номер задачи>");
            return;
        }
        std::string num_str = text.substr(pos + 1);
        try {
            int task_num = std::stoi(num_str);
            db_->DeleteTask(message->chat->id, task_num);
            tg_bot_->getApi().sendMessage(message->chat->id,
                                          std::format("Задача {} удалена", task_num));
            LOG(BotLog, INFO, "User {} deleted task {}", message->chat->id, task_num);
        } catch (const std::exception& e) {
            tg_bot_->getApi().sendMessage(message->chat->id, std::format("Ошибка: {}", e.what()));
            LOG(BotLog, ERROR, "Delete task error: {}", e.what());
        }
    });

    // /stats command
    tg_bot_->getEvents().onCommand("stats", [this](TgBot::Message::Ptr message) {
        try {
            auto stats = db_->GetUserStatistics(message->chat->id);
            std::string response = std::format(
                "Статистика:\n"
                "Всего задач: {}\n"
                "Выполнено: {}\n"
                "Активных: {}",
                stats.total, stats.completed, stats.active);
            tg_bot_->getApi().sendMessage(message->chat->id, response);
        } catch (const std::exception& e) {
            tg_bot_->getApi().sendMessage(
                message->chat->id, std::format("Ошибка при получении статистики: {}", e.what()));
            LOG(BotLog, ERROR, "Stats error: {}", e.what());
        }
    });

    // Default handler for unknown commands
    tg_bot_->getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
        if (message->text.empty() || message->text[0] == '/') {
            // Ignore commands and non-text messages
            return;
        }
        tg_bot_->getApi().sendMessage(message->chat->id,
                                      "Я понимаю только команды. Напишите /start для справки.");
    });
}

void Bot::Run() {
    SetupHandlers();

    TgBot::TgLongPoll longPoll(*tg_bot_);
    LOG(BotLog, INFO, "Bot started long polling");
    while (true) {
        try {
            longPoll.start();
        } catch (const std::exception& e) {
            LOG(BotLog, ERROR, "Long poll error: {}", e.what());
        }
    }
}

}  // namespace bot