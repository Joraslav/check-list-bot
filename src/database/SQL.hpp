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
    inline static constexpr std::string_view INSERT_USER =
        R"(INSERT INTO users (user_id, user_name) VALUES (?, ?))";

    inline static constexpr std::string_view INSERT_TASK =
        R"(INSERT INTO tasks (user_id, text, status) VALUES (?, ?, ?) RETURNING id)";

    inline static constexpr std::string_view SELECT_IS_USER_EXIST =
        R"(SELECT 1 FROM users WHERE user_id = ? LIMIT 1)";

    inline static constexpr std::string_view SELECT_USER =
        R"(SELECT id FROM users WHERE user_id = ?)";

    inline static constexpr std::string_view SELECT_TASKS_ALL =
        R"(SELECT id, text, status FROM tasks WHERE user_id = ? ORDER BY id)";

    inline static constexpr std::string_view SELECT_TASKS_BY_STATUS =
        R"(SELECT id, text, status FROM tasks WHERE user_id = ? AND status = ? ORDER BY id)";

    inline static constexpr std::string_view UPDATE_STATUS =
        R"(UPDATE tasks SET status = ? WHERE user_id = ? AND id = ?)";

    inline static constexpr std::string_view EDIT_TASK =
        R"(UPDATE tasks SET text = ? WHERE user_id = ? AND id = ?)";

    inline static constexpr std::string_view DELETE_TASK =
        R"(DELETE FROM tasks WHERE user_id = ? AND id = ?)";

    inline static constexpr std::string_view DELETE_ALL_USER_TASKS =
        R"(DELETE FROM tasks WHERE user_id = ?)";

    inline static constexpr std::string_view VERIFY_TASK_OWNERSHIP =
        R"(SELECT 1 FROM tasks WHERE id = ? AND user_id = ?)";

    inline static constexpr std::string_view GET_USER_STATISTICS =
        R"(SELECT u.user_id, COUNT(id), SUM(status), COUNT(id) - SUM(status)
        FROM users AS u JOIN tasks AS t ON u.user_id = t.user_id GROUP BY u.user_id)";
};

}  // namespace database
