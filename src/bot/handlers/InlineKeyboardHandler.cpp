#include "InlineKeyboardHandler.hpp"

#include "Bot.hpp"
#include "Log.hpp"

#include <string>

DEFINE_LOG_CATEGORY_STATIC(InlineKeyboardHandlerLog);

namespace bot {

using std::string_literals::operator""s;

void InlineKeyboardHandler::Register(TgBot::Bot& bot) {
    bot.getEvents().onCallbackQuery([&bot](CallbackQuery::Ptr query) { OnCallback(bot, query); });

    LOG(InlineKeyboardHandlerLog, INFO, "InlineKeyboardHandler registered");
}

void InlineKeyboardHandler::OnCallback(TgBot::Bot& bot, CallbackQuery::Ptr query) {
    std::string data = query->data;
    std::string response;
    if (data.starts_with("task_")) {
        int taskId = std::stoi(data.substr(5));
        response = "Selected task #"s.append(std::to_string(taskId));
    } else if (data.starts_with("confirm_")) {
        std::string answer = data.substr(8);
        response = "You chose "s.append(answer);
    } else if (data.starts_with("page_")) {
        int page = std::stoi(data.substr(5));
        response = "Page "s.append(std::to_string(page));
    } else if (data == "cancel"s) {
        response = "Cancelled."s;
    } else if (data == "close"s) {
        // Just answer the callback without sending a message
        try {
            bot.getApi().answerCallbackQuery(query->id);
        } catch (const TgException& e) {
            LOG(InlineKeyboardHandlerLog, ERROR, "Failed to answer callback: {}", e.what());
        }
        return;
    } else {
        response = "Unknown callback: "s.append(data);
    }

    try {
        bot.getApi().answerCallbackQuery(query->id, response);
        // Optionally edit the message
        // bot.getApi().editMessageText(...);
    } catch (const TgException& e) {
        LOG(InlineKeyboardHandlerLog, ERROR, "Failed to answer callback: {}", e.what());
    }
}

}  // namespace bot