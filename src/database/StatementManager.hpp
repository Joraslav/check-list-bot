#pragma once

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Exception.h"
#include "SQLiteCpp/Statement.h"

#include <memory>
#include <unordered_map>

namespace database {

/**
 * @brief Enum for statement types
 */
enum class StatementType {
    INSERT_USER,
    INSERT_TASK,
    SELECT_IS_USER_EXIST,
    SELECT_USER,
    SELECT_TASKS_ALL,
    SELECT_TASKS_BY_STATUS,
    UPDATE_STATUS,
    EDIT_TASK,
    DELETE_TASK,
    DELETE_ALL_USER_TASKS,
    VERIFY_TASK_OWNERSHIP,
    GET_USER_STATISTICS,
    COUNT
};

/**
 * @brief Class for managing SQL statements
 */
class StatementManager final {
 public:
    using Database = SQLite::Database;
    using Statement = SQLite::Statement;
    using Exception = SQLite::Exception;

    explicit StatementManager(Database& db);
    ~StatementManager() = default;

    StatementManager(const StatementManager&) = delete;
    StatementManager& operator=(const StatementManager&) = delete;

    /**
     * @brief Returns SQL statement by type
     * @param type type of statement `StatementType`
     * @return `Statement&` reference to SQL statement
     * @throws `std::out_of_range` if statement type is not found
     */
    Statement& Get(StatementType type);

    /**
     * @brief Returns prepared SQL statement by type (const version)
     */
    const Statement& Get(StatementType type) const;

    /**
     * @brief Resets all SQL statements
     */
    void ResetAll();

 private:
    /**
     * @brief Database connection
     */
    Database& db_;

    /**
     * @brief Storeged SQL statements
     */
    std::unordered_map<StatementType, std::unique_ptr<Statement>> statements_;

    /**
     * @brief Prepare SQL statements
     */
    void PrepareStatements();
};

}  // namespace database