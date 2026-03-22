#pragma once

#include "Task.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace database {

/**
 * @brief Interface for database connection
 */
class DatabaseConnection {
 public:
    DatabaseConnection() = default;

    virtual ~DatabaseConnection() = default;

    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    /**
     * @brief Add user to database
     * @param user_id ID user in database
     * @param user_name Name of user
     * @throw `std::runtime_error` if user is not added
     */
    virtual void AddUser(int64_t user_id, const std::string& user_name) = 0;

    /**
     * @brief Checks the connection to the database
     * @return `True` if connetion is established, `False` is not established
     */
    virtual bool IsConnected() noexcept = 0;

    /**
     * @brief Add task to database
     * @param user_id ID user in database
     * @param text Text of task
     * @param status Status of task. Default is ACTIVE
     * @return `std::optional<int64_t>` - The ID of the created task, or `std::nullopt`
     *         if the task could not be added
     */
    virtual std::optional<int64_t> AddTask(int64_t user_id, const std::string& text,
                                           TaskStatus status = TaskStatus::ACTIVE) = 0;

    /**
     * @brief Get ALL users tasks
     * @param user_id ID user in database
     * @return `std::optional<TaskList>` - Vector of tasks if exists
     */
    virtual std::optional<TaskList> GetAllTasks(int64_t user_id) = 0;

    /**
     * @brief Get active or completed tasks
     * @param user_id ID user in database
     * @param status Status of task
     * @return `std::optional<TaskList>` - Vector of tasks if exists
     */
    virtual std::optional<TaskList> GetActiveOrCompletedTasks(int64_t user_id,
                                                              TaskStatus status) = 0;

    /**
     * @brief Get user statistics
     * @param user_id ID user in database
     * @return `TaskStatistics` - Struct of statistics
     */
    virtual TaskStatistics GetUserStatistics(int64_t user_id) = 0;

    /**
     * @brief Update task status
     * @param user_id ID user in database (to verify ownership)
     * @param task_id ID task in database
     * @param new_status New task status
     */
    virtual void UpdateTaskStatus(int64_t user_id, int64_t task_id, TaskStatus new_status) = 0;

    /**
     * @brief Edit existing task text
     * @param user_id ID user in database (to verify ownership)
     * @param task_id ID task in database
     * @param new_text New task text
     */
    virtual void EditTask(int64_t user_id, int64_t task_id, const std::string& new_text) = 0;

    /**
     * @brief Delete task from database
     * @param user_id ID user in database (to verify ownership)
     * @param task_id ID task in database
     * @warning The operation is irreversible!
     */
    virtual void DeleteTask(int64_t user_id, int64_t task_id) = 0;

    /**
     * @brief Delete all users tasks
     * @param user_id ID user in database
     * @warning The operation is irreversible!
     */
    virtual void DeleteAllUserTasks(int64_t user_id) = 0;

    /**
     * @brief Begin transaction
     * @warning The operation is irreversible
     */
    virtual void BeginTransaction() = 0;

    /**
     * @brief Commit transaction
     */
    virtual void CommitTransaction() = 0;

    /**
     * @brief Rollback transaction
     */
    virtual void RollbackTransaction() = 0;

 protected:
    /**
     * @brief Accessor for database mutex in derived classes
     */
    std::recursive_mutex& DbMutex() noexcept { return db_mutex_; }

 private:
    mutable std::recursive_mutex db_mutex_;  ///< Mutex for thread safety
};

}  // namespace database
