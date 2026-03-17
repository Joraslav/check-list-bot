#include "SlashCommandKeyboard.hpp"

#include <format>

namespace bot {

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
        case Start:
            return "Initialize the bot and start using it. Sets up your profile.";
        case Help:
            return "Display help message with all available commands.";
        case List:
            return "Show all your tasks with their current status.";
        case Add:
            return "Add a new task. Follow the prompts to enter task details.";
        case Delete:
            return "Delete an existing task. Select from the list.";
        case Done:
            return "Mark a task as completed. Select from the list.";
        case Cancel:
            return "Cancel the current operation and return to the main menu.";
    }
    return "Unknown command.";
}

std::string SlashCommandKeyboard::GetCommandWithSlash(SlashCommand cmd) {
    return std::format("/{}", GetCommandName(cmd));
}

}  // namespace bot
