#include "InlineKeyboardHandler.hpp"

#include "../Bot.hpp"
#include "../../log/Log.hpp"

DEFINE_LOG_CATEGORY_STATIC(InlineKeyboardHandlerLog);

namespace bot
{

void InlineKeyboardHandler::Register(TgBot::Bot& bot)
{
    bot.getEvents().onCallbackQuery([&bot](TgBot::CallbackQuery::Ptr query) {
        OnCallback(bot, query);
    });

    LOG(InlineKeyboardHandlerLog, INFO, "InlineKeyboardHandler registered");
}

void InlineKeyboardHandler::OnCallback(TgBot::Bot& bot, TgBot::CallbackQuery::Ptr query)
{
    std::string data = query->data;
    std::string response;
    if (data.starts_with("task_")) {
        int taskId = std::stoi(data.substr(5));
        response = "Selected task #" + std::to_string(taskId);
    } else if (data.starts_with("confirm_")) {
        std::string answer = data.substr(8);
        response = "You chose " + answer;
    } else if (data.starts_with("page_")) {
        int page = std::stoi(data.substr(5));
        response = "Page " + std::to_string(page);
    } else if (data == "cancel") {
        response = "Cancelled.";
    } else if (data == "close") {
        // Just answer the callback without sending a message
        try {
            bot.getApi().answerCallbackQuery(query->id);
        } catch (const TgBot::TgException& e) {
            LOG(InlineKeyboardHandlerLog, ERROR, "Failed to answer callback: {}", e.what());
        }
        return;
    } else {
        response = "Unknown callback: " + data;
    }

    try {
        bot.getApi().answerCallbackQuery(query->id, response);
        // Optionally edit the message
        // bot.getApi().editMessageText(...);
    } catch (const TgBot::TgException& e) {
        LOG(InlineKeyboardHandlerLog, ERROR, "Failed to answer callback: {}", e.what());
    }
}

} // namespace bot