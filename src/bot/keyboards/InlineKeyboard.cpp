#include "InlineKeyboard.hpp"

using namespace database;

namespace bot {

TgBot::InlineKeyboardMarkup::Ptr InlineKeyboard::CreateTaskSelection(
    const TaskList& tasks) {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    for (const Task& task : tasks) {
        std::vector<TgBot::InlineKeyboardButton::Ptr> row;
        auto button = std::make_shared<TgBot::InlineKeyboardButton>();
        button->text = task.text;
        button->callbackData = "task_" + std::to_string(task.user_id);
        row.push_back(button);
        keyboard->inlineKeyboard.push_back(row);
    }
    // Add a "Cancel" button at the end
    std::vector<TgBot::InlineKeyboardButton::Ptr> cancelRow;
    auto cancelButton = std::make_shared<TgBot::InlineKeyboardButton>();
    cancelButton->text = "Cancel";
    cancelButton->callbackData = "cancel";
    cancelRow.push_back(cancelButton);
    keyboard->inlineKeyboard.push_back(cancelRow);
    return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr InlineKeyboard::CreateConfirmation(
    const std::string& callback_data_prefix) {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;
    auto yesButton = std::make_shared<TgBot::InlineKeyboardButton>();
    yesButton->text = "Yes";
    yesButton->callbackData = callback_data_prefix + "_yes";
    row.push_back(yesButton);

    auto noButton = std::make_shared<TgBot::InlineKeyboardButton>();
    noButton->text = "No";
    noButton->callbackData = callback_data_prefix + "_no";
    row.push_back(noButton);

    keyboard->inlineKeyboard.push_back(row);
    return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr InlineKeyboard::CreatePagination(
    int current_page, int total_pages, const std::string& callback_prefix) {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;

    // Previous button
    if (current_page > 1) {
        auto prevButton = std::make_shared<TgBot::InlineKeyboardButton>();
        prevButton->text = "◀ Previous";
        prevButton->callbackData = callback_prefix + "_" + std::to_string(current_page - 1);
        row.push_back(prevButton);
    }

    // Page indicator
    auto pageButton = std::make_shared<TgBot::InlineKeyboardButton>();
    pageButton->text = std::to_string(current_page) + "/" + std::to_string(total_pages);
    pageButton->callbackData = "page_current";
    row.push_back(pageButton);

    // Next button
    if (current_page < total_pages) {
        auto nextButton = std::make_shared<TgBot::InlineKeyboardButton>();
        nextButton->text = "Next ▶";
        nextButton->callbackData = callback_prefix + "_" + std::to_string(current_page + 1);
        row.push_back(nextButton);
    }

    if (!row.empty()) {
        keyboard->inlineKeyboard.push_back(row);
    }

    // Add a "Close" button below
    std::vector<TgBot::InlineKeyboardButton::Ptr> closeRow;
    auto closeButton = std::make_shared<TgBot::InlineKeyboardButton>();
    closeButton->text = "Close";
    closeButton->callbackData = "close";
    closeRow.push_back(closeButton);
    keyboard->inlineKeyboard.push_back(closeRow);

    return keyboard;
}

TgBot::InlineKeyboardButton::Ptr InlineKeyboard::CreateButton(const std::string& text,
                                                              const std::string& callback_data) {
    auto button = std::make_shared<TgBot::InlineKeyboardButton>();
    button->text = text;
    button->callbackData = callback_data;
    return button;
}

}  // namespace bot