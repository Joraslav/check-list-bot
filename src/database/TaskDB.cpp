#include "TaskDB.hpp"

#include "Log.hpp"
#include "magic_enum/magic_enum.hpp"
#include "ScopedStatement.hpp"
#include "SQL.hpp"
#include "Utils.hpp"

#include <format>
#include <ranges>
#include <stdexcept>

DEFINE_LOG_CATEGORY_STATIC(Console);
DEFINE_LOG_CATEGORY_STATIC(File);

namespace database {

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

TaskDB::TaskDB() : TaskDB(":memory:") {}

TaskDB::TaskDB(std::string_view db_path) : db_path_(db_path) {
    try {
        db_ = std::make_unique<Database>(db_path_.c_str(), SQLite::OPEN_READWRITE);
        statement_manager_ = std::make_unique<StatementManager>(*db_);

        db_->exec("PRAGMA foreign_keys = ON"s);
        db_->exec("PRAGMA journal_mode = WAL"s);
    } catch (const Exception& e) {
        throw std::runtime_error(std::format("Database failed to open: {}", e.what()));
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(std::format("Failed: {}", e.what()));
    }
}

bool TaskDB::IsUserExist(int64_t user_id) {
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

// TODO: Добавить проверку переменной text на SQL-инъекции
int64_t TaskDB::AddTask(int64_t user_id, const std::string& text, TaskStatus status) {
    ScopedStatement stmt(statement_manager_->Get(StatementType::INSERT_TASK));

    try {
        stmt->bind(1, user_id);
        stmt->bindNoCopy(2, text);
        stmt->bind(3, static_cast<int>(status));

        if (stmt->executeStep()) {
            const int64_t task_id = stmt->getColumn(0).getInt64();
            return task_id;
        }

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in AddTask: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in AddTask function: "s.append(e.what()));
    }
    return -1;
}

std::optional<TaskList> TaskDB::GetAllTasks(int64_t user_id) {
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
    ScopedStatement stmt(statement_manager_->Get(StatementType::GET_USER_STATISTICS));
    try {
        while (stmt->executeStep()) {
            const int64_t row_user_id = stmt->getColumn(0).getInt64();
            if (row_user_id == user_id) {
                const TaskStatistics stats{.total = stmt->getColumn(1).getInt(),
                                           .completed = stmt->getColumn(2).getInt(),
                                           .active = stmt->getColumn(3).getInt()};
                return stats;
            }
        }
        return TaskStatistics{0, 0, 0};
    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in GetUserStatistics: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in GetUserStatistics function: "s.append(e.what()));
    }
}

void TaskDB::UpdateTaskStatus(int64_t user_id, int64_t task_id, TaskStatus new_status) {
    ScopedStatement stmt(statement_manager_->Get(StatementType::UPDATE_STATUS));

    try {
        stmt->bind(1, static_cast<int>(new_status));
        stmt->bind(2, user_id);
        stmt->bind(3, task_id);

        stmt->exec();
    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in UpdateTaskStatus: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in UpdateTaskStatus function: "s.append(e.what()));
    }
}

void TaskDB::EditTask(int64_t user_id, int64_t task_id, const std::string& new_text) {
    ScopedStatement edit_stmt(statement_manager_->Get(StatementType::EDIT_TASK));
    ScopedStatement verify_stmt(statement_manager_->Get(StatementType::VERIFY_TASK_OWNERSHIP));

    try {
        verify_stmt->bind(1, task_id);
        verify_stmt->bind(2, user_id);
        if (!verify_stmt->executeStep()) {
            throw std::invalid_argument(
                std::format("Invalid task {} for user {}", task_id, user_id));
        }

        edit_stmt->bindNoCopy(1, new_text);
        edit_stmt->bind(2, user_id);
        edit_stmt->bind(3, task_id);

        edit_stmt->exec();

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in EditTask: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in EditTask function: "s.append(e.what()));
    }
}

void TaskDB::DeleteTask(int64_t user_id, int64_t task_id) {
    ScopedStatement delete_stmt(statement_manager_->Get(StatementType::DELETE_TASK));
    ScopedStatement verify_stmt(statement_manager_->Get(StatementType::VERIFY_TASK_OWNERSHIP));

    try {
        verify_stmt->bind(1, task_id);
        verify_stmt->bind(2, user_id);
        if (!verify_stmt->executeStep()) {
            throw std::invalid_argument(
                std::format("Invalid task {} for user {}", task_id, user_id));
        }

        delete_stmt->bind(1, user_id);
        delete_stmt->bind(2, task_id);

        delete_stmt->exec();

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in DeleteTask: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in DeleteTask function: "s.append(e.what()));
    }
}

void TaskDB::DeleteAllUserTasks(int64_t user_id) {
    ScopedStatement stmt(statement_manager_->Get(StatementType::DELETE_ALL_USER_TASKS));

    try {
        stmt->bind(1, user_id);
        stmt->exec();

    } catch (const Exception& e) {
        throw std::runtime_error("SQL error in DeleteAllUserTasks: "s.append(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error in DeleteAllUserTasks function: "s.append(e.what()));
    }
}

bool TaskDB::IsConnected() noexcept { return db_ != nullptr; }

void TaskDB::BeginTransaction() { db_->exec("BEGIN TRANSACTION"); }

void TaskDB::CommitTransaction() { db_->exec("COMMIT"); }

void TaskDB::RollbackTransaction() { db_->exec("ROLLBACK"); }

}  // namespace database
