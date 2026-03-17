#pragma once

#include "tgbot/types/ReplyKeyboardMarkup.h"
#include "tgbot/types/ReplyKeyboardRemove.h"

namespace bot {

class ReplyKeyboard final {
 public:
    using ReplyKeyboardMarkup = TgBot::ReplyKeyboardMarkup;
    using ReplyKeyboardRemove = TgBot::ReplyKeyboardRemove;
    using KeyboardButton = TgBot::KeyboardButton;

    /**
     * @brief Create a main menu reply keyboard.
     * @return TgBot::ReplyKeyboardMarkup::Ptr
     */
    static ReplyKeyboardMarkup::Ptr CreateMainMenu();

    /**
     * @brief Create a remove reply keyboard (hides the keyboard).
     * @return TgBot::ReplyKeyboardRemove::Ptr
     */
    static ReplyKeyboardRemove::Ptr CreateRemoveKeyboard();

    /**
     * @brief Create a keyboard for task list actions.
     * @return TgBot::ReplyKeyboardMarkup::Ptr
     */
    static ReplyKeyboardMarkup::Ptr CreateTaskActions();

    /**
     * @brief Create a yes/no keyboard.
     * @return TgBot::ReplyKeyboardMarkup::Ptr
     */
    static ReplyKeyboardMarkup::Ptr CreateYesNoKeyboard();
};

}  // namespace bot
