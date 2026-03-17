#include "SlashCommandKeyboard.hpp"

#include <cctype>
#include <format>

namespace bot {

bool SlashCommandKeyboard::StartsWithSlash(std::string_view str) noexcept {
    return !str.empty() && str.front() == '/';
}

std::string SlashCommandKeyboard::ToUpper(std::string_view str) noexcept {
    std::string upper_str{str};
    for (char& ch : upper_str) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return upper_str;
}

std::string SlashCommandKeyboard::ToLower(std::string_view str) noexcept {
    std::string lower_str{str};
    for (char& ch : lower_str) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return lower_str;
}

std::string SlashCommandKeyboard::GetCommandName(SlashCommand cmd) {
    return ToLower(magic_enum::enum_name(cmd));
}

std::optional<SlashCommand> SlashCommandKeyboard::ParseCommand(
    std::string_view command_str) noexcept {
    if (StartsWithSlash(command_str)) {
        command_str.remove_prefix(1);
    }
    const std::string upper_command = ToUpper(command_str);
    return magic_enum::enum_cast<SlashCommand>(upper_command);
}

std::string SlashCommandKeyboard::GetHelpText() {
    return R"(
╔════════════════════════════════════════╗
║      📋 CHECK-LIST BOT HELP 📋         ║
╠════════════════════════════════════════╣
║ /start  - Initialize the bot           ║
║ /help   - Show this help message       ║
║ /list   - Display your tasks           ║
║ /add    - Add a new task               ║
║ /delete - Delete a task                ║
║ /done   - Mark a task as completed     ║
║ /cancel - Cancel current operation     ║
╚════════════════════════════════════════╝
)";
}

std::string SlashCommandKeyboard::GetCommandHelp(SlashCommand cmd) {
    using enum SlashCommand;
    switch (cmd) {
        case START:
            return "Initialize the bot and start using it. Sets up your profile.";
        case HELP:
            return "Display help message with all available commands.";
        case LIST:
            return "Show all your tasks with their current status.";
        case ADD:
            return "Add a new task. Follow the prompts to enter task details.";
        case DELETE:
            return "Delete an existing task. Select from the list.";
        case DONE:
            return "Mark a task as completed. Select from the list.";
        case CANCEL:
            return "Cancel the current operation and return to the main menu.";
    }
    return "Unknown command.";
}

std::string SlashCommandKeyboard::GetCommandWithSlash(SlashCommand cmd) {
    return std::format("/{}", GetCommandName(cmd));
}

}  // namespace bot
