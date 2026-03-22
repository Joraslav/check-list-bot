#pragma once

#include <array>
#include <string_view>

namespace database {

class SQL {
 public:
    SQL() = default;
    ~SQL() = default;
    SQL(const SQL&) = delete;
    SQL& operator=(const SQL&) = delete;

    /**
     * @brief Returns prepared statements
     * @return `std::array<std::string_view>` - Array of prepared statements
     */
    static constexpr std::array<std::string_view, 12> GetPreparedStatements() {
        return {INSERT_USER,           INSERT_TASK,
                SELECT_IS_USER_EXIST,  SELECT_USER,
                SELECT_TASKS_ALL,      SELECT_TASKS_BY_STATUS,
                UPDATE_STATUS,         EDIT_TASK,
                DELETE_TASK,           DELETE_ALL_USER_TASKS,
                VERIFY_TASK_OWNERSHIP, GET_USER_STATISTICS};
    }

 private:
    /**
     * @brief Insert new user to `users` table
     * @param user_id ID user in database
     * @param user_name Name of user
     */
    static constexpr std::string_view INSERT_USER =
        R"(INSERT INTO users (user_id, user_name) VALUES (?, ?))";

    /**
     * @brief Insert new task to `tasks` table
     * @param user_id ID of the user who owns the task
     * @param text Text of the task
     * @param status Status of the task
     */
    static constexpr std::string_view INSERT_TASK =
        R"(INSERT INTO tasks (user_id, text, status) VALUES (?, ?, ?) RETURNING id)";

    /**
     * @brief Check if a user exists in the `users` table
     * @param user_id ID of the user to check
     */
    static constexpr std::string_view SELECT_IS_USER_EXIST =
        R"(SELECT 1 FROM users WHERE user_id = ? LIMIT 1)";

    /**
     * @brief Select a user from the `users` table
     * @param user_id ID of the user to select
     */
    static constexpr std::string_view SELECT_USER = R"(SELECT id FROM users WHERE user_id = ?)";

    /**
     * @brief Select all tasks for a user from the `tasks` table
     * @param user_id ID of the user to select tasks for
     */
    static constexpr std::string_view SELECT_TASKS_ALL =
        R"(SELECT id, text, status FROM tasks WHERE user_id = ? ORDER BY id)";

    /**
     * @brief Select tasks by status for a user from the `tasks` table
     * @param user_id ID of the user to select tasks for
     * @param status Status of the tasks to select
     */
    static constexpr std::string_view SELECT_TASKS_BY_STATUS =
        R"(SELECT id, text, status FROM tasks WHERE user_id = ? AND status = ? ORDER BY id)";

    /**
     * @brief Update the status of a task in the `tasks` table
     * @param status New status of the task
     * @param user_id ID of the user who owns the task
     * @param task_id ID of the task to update
     */
    static constexpr std::string_view UPDATE_STATUS =
        R"(UPDATE tasks SET status = ? WHERE user_id = ? AND id = ?)";

    /**
     * @brief Edit the text of a task in the `tasks` table
     * @param text New text of the task
     * @param user_id ID of the user who owns the task
     * @param task_id ID of the task to edit
     */
    static constexpr std::string_view EDIT_TASK =
        R"(UPDATE tasks SET text = ? WHERE user_id = ? AND id = ?)";

    /**
     * @brief Delete a task from the `tasks` table
     * @param user_id ID of the user who owns the task
     * @param task_id ID of the task to delete
     */
    static constexpr std::string_view DELETE_TASK =
        R"(DELETE FROM tasks WHERE user_id = ? AND id = ?)";

    /**
     * @brief Delete all tasks for a user from the `tasks` table
     * @param user_id ID of the user whose tasks to delete
     */
    static constexpr std::string_view DELETE_ALL_USER_TASKS =
        R"(DELETE FROM tasks WHERE user_id = ?)";

    /**
     * @brief Verify the ownership of a task
     * @param task_id ID of the task to verify
     * @param user_id ID of the user to verify ownership
     */
    static constexpr std::string_view VERIFY_TASK_OWNERSHIP =
        R"(SELECT 1 FROM tasks WHERE id = ? AND user_id = ?)";

    /**
     * @brief Get statistics for a user from the `tasks` table
     * @param user_id ID of the user to get statistics for
     */
    static constexpr std::string_view GET_USER_STATISTICS =
        R"(SELECT 
        COUNT(*) as total,
        SUM(CASE WHEN status = 1 THEN 1 ELSE 0 END) as completed,
        SUM(CASE WHEN status = 0 THEN 1 ELSE 0 END) as active
        FROM tasks WHERE user_id = ?)";
};

}  // namespace database
