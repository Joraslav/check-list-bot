#include "ReplyKeyboard.hpp"

#include <memory>
#include <string>
#include <vector>

namespace bot {

using std::string_literals::operator""s;
using ReplyKeyboardMarkup = ReplyKeyboard::ReplyKeyboardMarkup;
using ReplyKeyboardRemove = ReplyKeyboard::ReplyKeyboardRemove;

ReplyKeyboardMarkup::Ptr ReplyKeyboard::CreateMainMenu() {
    auto keyboard = std::make_shared<ReplyKeyboardMarkup>();
    keyboard->resizeKeyboard = true;
    keyboard->oneTimeKeyboard = false;
    keyboard->selective = false;

    std::vector<KeyboardButton::Ptr> row0;
    auto btn_start = std::make_shared<KeyboardButton>();
    btn_start->text = "start"s;
    row0.push_back(btn_start);
    auto btn_help = std::make_shared<KeyboardButton>();
    btn_help->text = "help"s;
    row0.push_back(btn_help);

    std::vector<KeyboardButton::Ptr> row1;
    auto btn_list = std::make_shared<KeyboardButton>();
    btn_list->text = "list"s;
    row1.push_back(btn_list);
    auto btn_add = std::make_shared<KeyboardButton>();
    btn_add->text = "add"s;
    row1.push_back(btn_add);

    std::vector<KeyboardButton::Ptr> row2;
    auto btn_delete = std::make_shared<KeyboardButton>();
    btn_delete->text = "delete"s;
    row2.push_back(btn_delete);
    auto btn_done = std::make_shared<KeyboardButton>();
    btn_done->text = "done"s;
    row2.push_back(btn_done);

    keyboard->keyboard.push_back(row0);
    keyboard->keyboard.push_back(row1);
    keyboard->keyboard.push_back(row2);

    return keyboard;
}

ReplyKeyboardRemove::Ptr ReplyKeyboard::CreateRemoveKeyboard() {
    auto remove = std::make_shared<ReplyKeyboardRemove>();
    remove->removeKeyboard = true;
    remove->selective = false;
    return remove;
}

ReplyKeyboardMarkup::Ptr ReplyKeyboard::CreateTaskActions() {
    auto keyboard = std::make_shared<ReplyKeyboardMarkup>();
    keyboard->resizeKeyboard = true;
    keyboard->oneTimeKeyboard = true;
    keyboard->selective = false;

    std::vector<KeyboardButton::Ptr> row0;
    auto btn_edit = std::make_shared<KeyboardButton>();
    btn_edit->text = "Edit"s;
    row0.push_back(btn_edit);
    auto btn_mark_done = std::make_shared<KeyboardButton>();
    btn_mark_done->text = "Mark Done"s;
    row0.push_back(btn_mark_done);

    std::vector<KeyboardButton::Ptr> row1;
    auto btn_back = std::make_shared<KeyboardButton>();
    btn_back->text = "Back"s;
    row1.push_back(btn_back);

    keyboard->keyboard.push_back(row0);
    keyboard->keyboard.push_back(row1);

    return keyboard;
}

ReplyKeyboardMarkup::Ptr ReplyKeyboard::CreateYesNoKeyboard() {
    auto keyboard = std::make_shared<ReplyKeyboardMarkup>();
    keyboard->resizeKeyboard = true;
    keyboard->oneTimeKeyboard = true;
    keyboard->selective = false;

    std::vector<KeyboardButton::Ptr> row0;
    auto btn_yes = std::make_shared<KeyboardButton>();
    btn_yes->text = "Yes"s;
    row0.push_back(btn_yes);
    auto btn_no = std::make_shared<KeyboardButton>();
    btn_no->text = "No"s;
    row0.push_back(btn_no);

    keyboard->keyboard.push_back(row0);
    return keyboard;
}

}  // namespace bot