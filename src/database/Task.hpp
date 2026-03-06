#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace database {

/**
 * @brief Enum for task status
 */
enum class TaskStatus { ACTIVE, COMPLETED };

struct Task {
    int64_t user_id;
    std::string text;
    TaskStatus status;

    explicit Task(int64_t p_user_id, const std::string& p_text, TaskStatus p_status)
        : user_id(p_user_id), text(p_text), status(p_status) {}

    explicit Task(int64_t p_user_id, std::string_view p_text, TaskStatus p_status)
        : user_id(p_user_id), text(p_text), status(p_status) {}
};

struct TaskStatistics {
    int total;
    int completed;
    int active;
};

}  // namespace database