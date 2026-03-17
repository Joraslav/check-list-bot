#pragma once

#include <magic_enum/magic_enum.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace bot {

/**
 * @brief Enumeration of available slash commands.
 */
enum class SlashCommand : unsigned char {
    START,   ///< /start - Initialize bot and user
    HELP,    ///< /help - Show help message
    LIST,    ///< /list - Show task list
    ADD,     ///< /add - Add new task
    DELETE,  ///< /delete - Delete a task
    DONE,    ///< /done - Mark task as done
    CANCEL,  ///< /cancel - Cancel current operation
};

/**
 * @brief Keyboard providing slash command information and helpers.
 */
class SlashCommandKeyboard final {
 public:
    SlashCommandKeyboard() = delete;

    /**
     * @brief Convert string to SlashCommand enum.
     * @param command_str Command string (without '/')
     * @return SlashCommand if valid, `std::nullopt` otherwise
     */
    static std::optional<SlashCommand> ParseCommand(std::string_view command_str) noexcept;

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
     * @brief Get the command name with slash prefix.
     * @param cmd The command
     * @return Command with '/' prefix
     */
    static std::string GetCommandWithSlash(SlashCommand cmd);

 private:
    /**
     * @brief Check if string starts with '/'
     * @param str Input string
     * @return `true` if string is not empty and starts with '/', `false` otherwise
     */
    static bool IsBeginSlash(std::string_view str) noexcept;

    /**
     * @brief Convert string to uppercase
     * @param str Input string
     * @return Uppercase string
     */
    static std::string ToUpper(std::string_view str) noexcept;

    /**
     * @brief Convert string to lowercase
     * @param str Input string
     * @return Lowercase string
     */
    static std::string ToLower(std::string_view str) noexcept;

    /**
     * @brief Get the name of a command.
     * @param cmd The command
     * @return Command name (without '/')
     */
    static std::string GetCommandName(SlashCommand cmd);
};

}  // namespace bot
