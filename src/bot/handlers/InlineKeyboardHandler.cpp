#include "InlineKeyboardHandler.hpp"

#include "Log.hpp"

#include <charconv>
#include <format>
#include <string>
#include <string_view>

DEFINE_LOG_CATEGORY_STATIC(InlineKeyboardHandlerLog);

namespace bot {

using std::string_view_literals::operator""sv;

void InlineKeyboardHandler::Register(TgBot::Bot& bot) {
    bot.getEvents().onCallbackQuery(
        [&bot](const CallbackQuery::Ptr& query) { OnCallback(bot, query); });

    LOG(InlineKeyboardHandlerLog, INFO, "InlineKeyboardHandler registered");
}

void InlineKeyboardHandler::OnCallback(TgBot::Bot& bot, const CallbackQuery::Ptr& query) {
    const std::string_view data = query->data;
    std::string response;

    auto parse_int = [](std::string_view str) -> int {
        int value = 0;
        std::from_chars(str.data(), str.data() + str.size(), value);
        return value;
    };

    if (data.starts_with("task_"sv)) {
        response = std::format("Selected task #{}", parse_int(data.substr(5)));
    } else if (data.starts_with("confirm_"sv)) {
        response = std::format("You chose {}", data.substr(8));
    } else if (data.starts_with("page_"sv)) {
        response = std::format("Page {}", parse_int(data.substr(5)));
    } else if (data == "cancel"sv) {
        response = "Cancelled.";
    } else if (data == "close"sv) {
        // Just answer the callback without sending a message
        try {
            bot.getApi().answerCallbackQuery(query->id);
        } catch (const TgException& e) {
            LOG(InlineKeyboardHandlerLog, ERROR, "Failed to answer callback: {}", e.what());
        }
        return;
    } else {
        response = std::format("Unknown callback: {}", data);
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