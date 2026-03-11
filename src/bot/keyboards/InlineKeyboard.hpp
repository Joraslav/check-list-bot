#pragma once

#include "tgbot/types/InlineKeyboardButton.h"
#include "tgbot/types/InlineKeyboardMarkup.h"

#include "Task.hpp"

namespace bot {

class InlineKeyboard {
 public:
    using InlineKeyboardMarkup = TgBot::InlineKeyboardMarkup;
    using InlineKeyboardButton = TgBot::InlineKeyboardButton;

    /**
     * @brief Create an inline keyboard for task selection.
     * @param tasks List of tasks.
     * @return TgBot::InlineKeyboardMarkup::Ptr
     */
    static InlineKeyboardMarkup::Ptr CreateTaskSelection(const database::TaskList& tasks);

    /**
     * @brief Create an inline keyboard for confirmation (Yes/No).
     * @param callback_data_prefix Prefix for callback data (e.g., "confirm_").
     * @return TgBot::InlineKeyboardMarkup::Ptr
     */
    static InlineKeyboardMarkup::Ptr CreateConfirmation(
        const std::string& callback_data_prefix = "confirm");

    /**
     * @brief Create an inline keyboard for pagination.
     * @param current_page Current page number.
     * @param total_pages Total pages.
     * @param callback_prefix Prefix for callback data (e.g., "page_").
     * @return TgBot::InlineKeyboardMarkup::Ptr
     */
    static InlineKeyboardMarkup::Ptr CreatePagination(
        int current_page, int total_pages, const std::string& callback_prefix = "page");

    /**
     * @brief Create a simple inline button.
     * @param text Button text.
     * @param callback_data Callback data.
     * @return TgBot::InlineKeyboardButton::Ptr
     */
    static InlineKeyboardButton::Ptr CreateButton(const std::string& text,
                                                         const std::string& callback_data);
};

}  // namespace bot
