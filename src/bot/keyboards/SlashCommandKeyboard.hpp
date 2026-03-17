#pragma once

#include <magic_enum/magic_enum.hpp>

#include <string>
#include <string_view>

namespace bot {

/**
 * @brief Enumeration of available slash commands.
 */
enum class SlashCommand : unsigned char {
    Start,   ///< /start - Initialize bot and user
    Help,    ///< /help - Show help message
    List,    ///< /list - Show task list
    Add,     ///< /add - Add new task
    Delete,  ///< /delete - Delete a task
    Done,    ///< /done - Mark task as done
    Cancel,  ///< /cancel - Cancel current operation
};

/**
 * @brief Keyboard providing slash command information and helpers.
 */
class SlashCommandKeyboard final {
 public:
    /**
     * @brief Convert string to SlashCommand enum.
     * @param command_str Command string (without '/')
     * @return SlashCommand if valid, std::nullopt otherwise
     */
    static constexpr auto ParseCommand(std::string_view command_str) noexcept {
        return magic_enum::enum_cast<SlashCommand>(command_str);
    }

    /**
     * @brief Get the help text for all available commands.
     * @return Help message string
     */
    static std::string GetHelpText();

    /**
     * @brief Get help text for a specific command.
     * @param cmd The command
     * @return Help message for the command
     */
    static std::string GetCommandHelp(SlashCommand cmd);

    /**
     * @brief Get the name of a command.
     * @param cmd The command
     * @return Command name (without '/')
     */
    static constexpr std::string_view GetCommandName(SlashCommand cmd) noexcept {
        return magic_enum::enum_name(cmd);
    }

    /**
     * @brief Get the command name with slash prefix.
     * @param cmd The command
     * @return Command with '/' prefix
     */
    static std::string GetCommandWithSlash(SlashCommand cmd);

    SlashCommandKeyboard() = delete;
};

}  // namespace bot
