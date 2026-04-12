#include "TaskDB.hpp"

#include "Log.hpp"
#include "ScopedStatement.hpp"

#include <exception>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

DEFINE_LOG_CATEGORY_STATIC(Console);
DEFINE_LOG_CATEGORY_STATIC(File);

namespace database {

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

TaskDB::TransactionGuard::TransactionGuard(TaskDB& task_db)
    : task_db_(&task_db), lock_(task_db.DbMutex()) {
    try {
        task_db_->db_->exec("BEGIN IMMEDIATE TRANSACTION");
        task_db_->guard_transaction_active_ = true;
        active_ = true;
    } catch (const Exception& e) {
        throw std::runtime_error(std::format("Failed to begin transaction: {}", e.what()));
    }
}

TaskDB::TransactionGuard::~TransactionGuard() noexcept {
    if (!active_ || task_db_ == nullptr) {
        return;
    }

    try {
        task_db_->db_->exec("ROLLBACK");
    } catch (const Exception& e) {
        LOG(Console, ERROR, "Failed to rollback transaction in destructor: {}", e.what());
    }
    task_db_->guard_transaction_active_ = false;
    active_ = false;
}

TaskDB::TransactionGuard::TransactionGuard(TransactionGuard&& other) noexcept
    : task_db_(other.task_db_), lock_(std::move(other.lock_)), active_(other.active_) {
    other.task_db_ = nullptr;
    other.active_ = false;
}

TaskDB::TransactionGuard& TaskDB::TransactionGuard::operator=(TransactionGuard&& other) noexcept {
    if (this != &other) {
        if (active_ && task_db_ != nullptr) {
            try {
                task_db_->db_->exec("ROLLBACK");
            } catch (const Exception& e) {
                LOG(Console, ERROR, "Failed to rollback transaction in move assignment: {}",
                    e.what());
            }
            active_ = false;
        }
        task_db_ = other.task_db_;
        lock_ = std::move(other.lock_);
        active_ = other.active_;
        other.task_db_ = nullptr;
        other.active_ = false;
    }
    return *this;
}

void TaskDB::TransactionGuard::Commit() {
    if (!active_ || task_db_ == nullptr) {
        throw std::logic_error("Transaction already finished");
    }

    try {
        task_db_->db_->exec("COMMIT");
        task_db_->guard_transaction_active_ = false;
        active_ = false;
        task_db_ = nullptr;
        lock_.unlock();
    } catch (const Exception& e) {
        throw std::runtime_error(std::format("Failed to commit transaction: {}", e.what()));
    }
}

void TaskDB::TransactionGuard::Rollback() {
    if (!active_ || task_db_ == nullptr) {
        return;
    }

    try {
        task_db_->db_->exec("ROLLBACK");
        task_db_->guard_transaction_active_ = false;
        active_ = false;
        task_db_ = nullptr;
        lock_.unlock();
    } catch (const Exception& e) {
        throw std::runtime_error(std::format("Failed to rollback transaction: {}", e.what()));
    }
}

bool TaskDB::TransactionGuard::IsActive() const noexcept { return active_; }

TaskDB::TransactionGuard TaskDB::CreateTransactionGuard() { return TransactionGuard(*this); }

void TaskDB::InitializeSchema() {
    const std::string schema = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER UNIQUE NOT NULL,
            user_name TEXT NOT NULL DEFAULT ''
        );
        
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            text TEXT NOT NULL,
            status INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
        );
        
        CREATE INDEX IF NOT EXISTS idx_tasks_user_id ON tasks(user_id);
        CREATE INDEX IF NOT EXISTS idx_tasks_user_status ON tasks(user_id, status);
    )";
    try {
        db_->exec(schema);
    } catch (const Exception& e) {
        throw std::runtime_error(std::format("Failed to initialize database schema: {}", e.what()));
    }
}

TaskDB::TaskDB() : TaskDB(":memory:") {}

TaskDB::TaskDB(std::string_view db_path) : db_path_(db_path) {
    try {
        db_ = std::make_unique<Database>(db_path_.c_str(), SQLite::OPEN_READWRITE |
                                                               SQLite::OPEN_CREATE |
                                                               SQLite::OPEN_FULLMUTEX);

        db_->exec("PRAGMA foreign_keys = ON"s);
        db_->exec("PRAGMA journal_mode = WAL"s);
        InitializeSchema();
        statement_manager_ = std::make_unique<StatementManager>(*db_);
    } catch (const Exception& e) {
        throw std::runtime_error(std::format("Failed to configure database: {}", e.what()));
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(std::format("Failed to initialize database: {}", e.what()));
    }
}

bool TaskDB::IsUserExist(int64_t user_id) {
    const std::scoped_lock lock(DbMutex());
    ScopedStatement stmt(statement_manager_->Get(StatementType::SELECT_IS_USER_EXIST));
    try {
        stmt->bind(1, user_id);
        const bool exist = stmt->executeStep();
        return exist;
    } catch (const Exception& e) {
        throw std::runtime_error("Error in SQL: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::format("Error checking user - {}: Error: {}"sv, user_id, e.what()));
    }
}

void TaskDB::AddUser(int64_t user_id, const std::string& user_name) {
    const std::scoped_lock lock(DbMutex());
    std::unique_ptr<TransactionGuard> transaction;
    if (!manual_transaction_active_ && !guard_transaction_active_) {
        transaction = std::make_unique<TransactionGuard>(*this);
    }
    ScopedStatement stmt(statement_manager_->Get(StatementType::INSERT_USER));

    try {
        stmt->bind(1, user_id);
        stmt->bindNoCopy(2, user_name);
        stmt->exec();
        if (transaction != nullptr) {
            transaction->Commit();
        }
    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in AddUser: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in AddUser function: "s.append(e.what()));
    }
}

std::optional<int64_t> TaskDB::AddTask(int64_t user_id, const std::string& text,
                                       TaskStatus status) {
    const std::scoped_lock lock(DbMutex());
    std::unique_ptr<TransactionGuard> transaction;
    if (!manual_transaction_active_ && !guard_transaction_active_) {
        transaction = std::make_unique<TransactionGuard>(*this);
    }
    ScopedStatement stmt(statement_manager_->Get(StatementType::INSERT_TASK));

    try {
        stmt->bind(1, user_id);
        stmt->bindNoCopy(2, text);
        stmt->bind(3, static_cast<int>(status));

        if (stmt->executeStep()) {
            const int64_t task_id = stmt->getColumn(0).getInt64();
            if (transaction != nullptr) {
                transaction->Commit();
            }
            return task_id;
        }

        if (transaction != nullptr) {
            transaction->Commit();
        }

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in AddTask: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in AddTask function: "s.append(e.what()));
    }
    return std::nullopt;
}

std::optional<TaskList> TaskDB::GetAllTasks(int64_t user_id) {
    const std::scoped_lock lock(DbMutex());
    ScopedStatement stmt(statement_manager_->Get(StatementType::SELECT_TASKS_ALL));

    try {
        stmt->bind(1, user_id);
        TaskList tasks;
        while (stmt->executeStep()) {
            // id is column 0, but we don't need it
            const std::string text = stmt->getColumn(1).getString();
            const int status_int = stmt->getColumn(2).getInt();
            const auto status = static_cast<TaskStatus>(status_int);
            tasks.emplace_back(user_id, text, status);
        }
        if (tasks.empty()) {
            return std::nullopt;
        }
        return tasks;

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in GetAllTasks: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in GetAllTasks function: "s.append(e.what()));
    }
}

std::optional<TaskList> TaskDB::GetActiveOrCompletedTasks(int64_t user_id, TaskStatus status) {
    const std::scoped_lock lock(DbMutex());
    ScopedStatement stmt(statement_manager_->Get(StatementType::SELECT_TASKS_BY_STATUS));

    try {
        stmt->bind(1, user_id);
        stmt->bind(2, static_cast<int>(status));
        TaskList tasks;

        while (stmt->executeStep()) {
            // id is column 0, but we don't need it
            const std::string text = stmt->getColumn(1).getString();
            const int status_int = stmt->getColumn(2).getInt();
            const auto status = static_cast<TaskStatus>(status_int);
            tasks.emplace_back(user_id, text, status);
        }
        if (tasks.empty()) {
            return std::nullopt;
        }
        return tasks;

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in GetActiveOrCompletedTasks: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in GetActiveOrCompletedTasks function: "s.append(e.what()));
    }
}

TaskStatistics TaskDB::GetUserStatistics(int64_t user_id) {
    const std::scoped_lock lock(DbMutex());
    ScopedStatement stmt(statement_manager_->Get(StatementType::GET_USER_STATISTICS));
    try {
        stmt->bind(1, user_id);
        if (stmt->executeStep()) {
            const TaskStatistics stats{.total = stmt->getColumn(0).getInt(),
                                       .completed = stmt->getColumn(1).getInt(),
                                       .active = stmt->getColumn(2).getInt()};
            return stats;
        }
        return TaskStatistics{.total = 0, .completed = 0, .active = 0};
    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in GetUserStatistics: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in GetUserStatistics function: "s.append(e.what()));
    }
}

void TaskDB::UpdateTaskStatus(int64_t user_id, int64_t task_id, TaskStatus new_status) {
    const std::scoped_lock lock(DbMutex());
    std::unique_ptr<TransactionGuard> transaction;
    if (!manual_transaction_active_ && !guard_transaction_active_) {
        transaction = std::make_unique<TransactionGuard>(*this);
    }
    ScopedStatement stmt(statement_manager_->Get(StatementType::UPDATE_STATUS));

    try {
        stmt->bind(1, static_cast<int>(new_status));
        stmt->bind(2, user_id);
        stmt->bind(3, task_id);

        stmt->exec();
        if (transaction != nullptr) {
            transaction->Commit();
        }
    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in UpdateTaskStatus: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in UpdateTaskStatus function: "s.append(e.what()));
    }
}

void TaskDB::EditTask(int64_t user_id, int64_t task_id, const std::string& new_text) {
    const std::scoped_lock lock(DbMutex());
    std::unique_ptr<TransactionGuard> transaction;
    if (!manual_transaction_active_ && !guard_transaction_active_) {
        transaction = std::make_unique<TransactionGuard>(*this);
    }
    ScopedStatement edit_stmt(statement_manager_->Get(StatementType::EDIT_TASK));

    try {
        edit_stmt->bindNoCopy(1, new_text);
        edit_stmt->bind(2, user_id);
        edit_stmt->bind(3, task_id);

        edit_stmt->exec();
        if (db_->getChanges() == 0) {
            // getChanges() == 0 can mean either "task not found" or "same value was set".
            // Do a follow-up ownership check to distinguish the two cases.
            ScopedStatement verify_stmt(
                statement_manager_->Get(StatementType::VERIFY_TASK_OWNERSHIP));
            verify_stmt->bind(1, task_id);
            verify_stmt->bind(2, user_id);
            if (!verify_stmt->executeStep()) {
                throw std::invalid_argument(
                    std::format("Invalid task {} for user {}", task_id, user_id));
            }
        }

        if (transaction != nullptr) {
            transaction->Commit();
        }

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in EditTask: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in EditTask function: "s.append(e.what()));
    }
}

void TaskDB::DeleteTask(int64_t user_id, int64_t task_id) {
    const std::scoped_lock lock(DbMutex());
    std::unique_ptr<TransactionGuard> transaction;
    if (!manual_transaction_active_ && !guard_transaction_active_) {
        transaction = std::make_unique<TransactionGuard>(*this);
    }
    ScopedStatement delete_stmt(statement_manager_->Get(StatementType::DELETE_TASK));

    try {
        delete_stmt->bind(1, user_id);
        delete_stmt->bind(2, task_id);

        delete_stmt->exec();
        if (db_->getChanges() == 0) {
            throw std::invalid_argument(
                std::format("Invalid task {} for user {}", task_id, user_id));
        }

        if (transaction != nullptr) {
            transaction->Commit();
        }

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in DeleteTask: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in DeleteTask function: "s.append(e.what()));
    }
}

void TaskDB::DeleteAllUserTasks(int64_t user_id) {
    const std::scoped_lock lock(DbMutex());
    std::unique_ptr<TransactionGuard> transaction;
    if (!manual_transaction_active_ && !guard_transaction_active_) {
        transaction = std::make_unique<TransactionGuard>(*this);
    }
    ScopedStatement stmt(statement_manager_->Get(StatementType::DELETE_ALL_USER_TASKS));

    try {
        stmt->bind(1, user_id);
        stmt->exec();

        if (transaction != nullptr) {
            transaction->Commit();
        }

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in DeleteAllUserTasks: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in DeleteAllUserTasks function: "s.append(e.what()));
    }
}

bool TaskDB::IsConnected() noexcept {
    const std::scoped_lock lock(DbMutex());
    return db_ != nullptr;
}

void TaskDB::BeginTransaction() {
    const std::scoped_lock lock(DbMutex());
    if (manual_transaction_active_) {
        throw std::logic_error("Transaction already active");
    }
    db_->exec("BEGIN IMMEDIATE TRANSACTION");
    manual_transaction_active_ = true;
}

void TaskDB::CommitTransaction() {
    const std::scoped_lock lock(DbMutex());
    if (!manual_transaction_active_) {
        throw std::logic_error("No active transaction");
    }
    db_->exec("COMMIT");
    manual_transaction_active_ = false;
}

void TaskDB::RollbackTransaction() {
    const std::scoped_lock lock(DbMutex());
    if (!manual_transaction_active_) {
        throw std::logic_error("No active transaction");
    }
    db_->exec("ROLLBACK");
    manual_transaction_active_ = false;
}

}  // namespace database
