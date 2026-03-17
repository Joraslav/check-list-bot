#include "InlineKeyboard.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace database;

namespace bot {

using std::string_literals::operator""s;
using InlineKeyboardMarkup = InlineKeyboard::InlineKeyboardMarkup;
using InlineKeyboardButton = InlineKeyboard::InlineKeyboardButton;

InlineKeyboardMarkup::Ptr InlineKeyboard::CreateTaskSelection(const TaskList& tasks) {
    auto keyboard = std::make_shared<InlineKeyboardMarkup>();
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        const Task& task = tasks[i];
        std::vector<InlineKeyboardButton::Ptr> row;
        auto button = std::make_shared<InlineKeyboardButton>();
        button->text = task.text;
        button->callbackData = "task_"s.append(std::to_string(i));
        row.push_back(button);
        keyboard->inlineKeyboard.push_back(row);
    }
    // Add a "Cancel" button at the end
    std::vector<InlineKeyboardButton::Ptr> cancel_row;
    auto cancel_button = std::make_shared<InlineKeyboardButton>();
    cancel_button->text = "Cancel"s;
    cancel_button->callbackData = "cancel"s;
    cancel_row.push_back(cancel_button);
    keyboard->inlineKeyboard.push_back(cancel_row);
    return keyboard;
}

InlineKeyboardMarkup::Ptr InlineKeyboard::CreateConfirmation(
    const std::string& callback_data_prefix) {
    auto keyboard = std::make_shared<InlineKeyboardMarkup>();
    std::vector<InlineKeyboardButton::Ptr> row;
    auto yes_button = std::make_shared<InlineKeyboardButton>();
    yes_button->text = "Yes"s;
    yes_button->callbackData = callback_data_prefix + "_yes"s;
    row.push_back(yes_button);

    auto no_button = std::make_shared<InlineKeyboardButton>();
    no_button->text = "No"s;
    no_button->callbackData = callback_data_prefix + "_no"s;
    row.push_back(no_button);

    keyboard->inlineKeyboard.push_back(row);
    return keyboard;
}

InlineKeyboardMarkup::Ptr InlineKeyboard::CreatePagination(int current_page, int total_pages,
                                                           const std::string& callback_prefix) {
    auto keyboard = std::make_shared<InlineKeyboardMarkup>();
    std::vector<InlineKeyboardButton::Ptr> row;

    // Previous button
    if (current_page > 1) {
        auto prev_button = std::make_shared<InlineKeyboardButton>();
        prev_button->text = "◀ Previous"s;
        prev_button->callbackData = callback_prefix + "_"s.append(std::to_string(current_page - 1));
        row.push_back(prev_button);
    }

    // Page indicator
    auto page_button = std::make_shared<InlineKeyboardButton>();
    page_button->text = std::to_string(current_page) + "/"s.append(std::to_string(total_pages));
    page_button->callbackData = "page_current"s;
    row.push_back(page_button);

    // Next button
    if (current_page < total_pages) {
        auto next_button = std::make_shared<InlineKeyboardButton>();
        next_button->text = "Next ▶"s;
        next_button->callbackData = callback_prefix + "_"s.append(std::to_string(current_page + 1));
        row.push_back(next_button);
    }

    if (!row.empty()) {
        keyboard->inlineKeyboard.push_back(row);
    }

    // Add a "Close" button below
    std::vector<InlineKeyboardButton::Ptr> close_row;
    auto close_button = std::make_shared<InlineKeyboardButton>();
    close_button->text = "Close"s;
    close_button->callbackData = "close"s;
    close_row.push_back(close_button);
    keyboard->inlineKeyboard.push_back(close_row);

    return keyboard;
}

InlineKeyboardButton::Ptr InlineKeyboard::CreateButton(const std::string& text,
                                                       const std::string& callback_data) {
    auto button = std::make_shared<InlineKeyboardButton>();
    button->text = text;
    button->callbackData = callback_data;
    return button;
}

}  // namespace bot