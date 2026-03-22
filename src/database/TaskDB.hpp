#pragma once

#include "DatabaseConnection.hpp"
#include "StatementManager.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace SQLite {
class Database;
class Statement;
class Exception;
}  // namespace SQLite

namespace database {

class TaskDB final : public DatabaseConnection {
 public:
    using Database = SQLite::Database;
    using Statement = SQLite::Statement;
    using Exception = SQLite::Exception;

    /**
     * @brief Constructs TaskDB with in-memory database
     */
    TaskDB();
    ~TaskDB() override = default;

    /**
     * @brief Constructs TaskDB with database at db_path
     * @param db_path Path to database
     */
    explicit TaskDB(std::string_view db_path);

    TaskDB(const TaskDB&) = delete;
    TaskDB& operator=(const TaskDB&) = delete;

    class TransactionGuard final {
     public:
        explicit TransactionGuard(TaskDB& task_db);
        ~TransactionGuard() noexcept;

        TransactionGuard(const TransactionGuard&) = delete;
        TransactionGuard& operator=(const TransactionGuard&) = delete;
        TransactionGuard(TransactionGuard&& other) noexcept;
        TransactionGuard& operator=(TransactionGuard&& other) noexcept;

        void Commit();
        void Rollback();
        [[nodiscard]] bool IsActive() const noexcept;

     private:
        TaskDB* task_db_ = nullptr;
        std::unique_lock<std::recursive_mutex> lock_;
        bool active_ = false;
    };

    /**
     * @brief Create RAII guard for atomic transaction in multithreaded scenario
     */
    [[nodiscard]] TransactionGuard CreateTransactionGuard();

    /**
     * @brief Check existing user in database
     * @param user_id ID user in database
     * @return `True` is user exist, `False` if don`t
     */
    bool IsUserExist(int64_t user_id);

    /**
     * @brief Add user to database
     * @param user_id ID user in database
     * @param user_name Name of user
     * @throw `std::runtime_error` if user is not added
     */
    void AddUser(int64_t user_id, const std::string& user_name) override;

    /**
     * @brief Checks the connection to the database
     * @return `True` if connetion is established, `False` is not established
     */
    bool IsConnected() noexcept override;

    /**
     * @brief Add task to database
     * @param user_id ID user in database
     * @param text Text of task
     * @param status Status of task. Default is ACTIVE
     * @return `std::optional<int64_t>` - The ID of the created task, or `std::nullopt` if the task was not added
     */
    std::optional<int64_t> AddTask(int64_t user_id, const std::string& text,
                                   TaskStatus status = TaskStatus::ACTIVE) override;

    /**
     * @brief Get ALL users tasks
     * @param user_id ID user in database
     * @return `std::optional<TaskList>` - Vector of tasks if exists
     */
    std::optional<TaskList> GetAllTasks(int64_t user_id) override;

    /**
     * @brief Get active or completed tasks
     * @param user_id ID user in database
     * @param status Status of task
     * @return `std::optional<TaskList>` - Vector of tasks if exists
     */
    std::optional<TaskList> GetActiveOrCompletedTasks(int64_t user_id, TaskStatus status) override;

    /**
     * @brief Get user statistics
     * @param user_id ID user in database
     * @return `TaskStatistics` - Struct of statistics
     */
    TaskStatistics GetUserStatistics(int64_t user_id) override;

    /**
     * @brief Update task status
     * @param user_id ID user in database (to verify ownership)
     * @param task_id ID task in database
     * @param new_status New task status
     */
    void UpdateTaskStatus(int64_t user_id, int64_t task_id, TaskStatus new_status) override;

    /**
     * @brief Edit existing task text
     * @param user_id ID user in database (to verify ownership)
     * @param task_id ID task in database
     * @param new_text New task text
     */
    void EditTask(int64_t user_id, int64_t task_id, const std::string& new_text) override;

    /**
     * @brief Delete task from database
     * @param user_id ID user in database (to verify ownership)
     * @param task_id ID task in database
     * @warning The operation is irreversible!
     */
    void DeleteTask(int64_t user_id, int64_t task_id) override;

    /**
     * @brief Delete all users tasks
     * @param user_id ID user in database
     * @warning The operation is irreversible!
     */
    void DeleteAllUserTasks(int64_t user_id) override;

    /**
     * @brief Begin transaction
     * @warning The operation is irreversible
     */
    void BeginTransaction() override;

    /**
     * @brief Commit transaction
     */
    void CommitTransaction() override;

    /**
     * @brief Rollback transaction
     */
    void RollbackTransaction() override;

 private:
    /**
     * @brief Database connection
     */
    std::unique_ptr<Database> db_;

    /**
     * @brief Path to database
     */
    std::string db_path_;

    /**
     * @brief Statement manager for use statements
     */
    std::unique_ptr<StatementManager> statement_manager_;

    /**
     * @brief Initialize database schema. Safe to call on startup; uses IF NOT EXISTS so it can be
     * invoked whenever the database is opened
     */
    void InitializeSchema();
};

}  // namespace database
