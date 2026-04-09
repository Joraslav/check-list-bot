#pragma once

#include "SQLiteCpp/Statement.h"

namespace database {

/**
 * @brief RAII wrapper for SQLite::Statement that automatically calls reset() on destruction.
 *
 * This class ensures that a SQLite statement is reset when it goes out of scope,
 * even if an exception is thrown. It provides transparent access to the underlying
 * statement via operator-> and operator*.
 */
class ScopedStatement final {
 public:
    using Statement = SQLite::Statement;

    /**
     * @brief Constructs a ScopedStatement taking ownership of a statement reference.
     * @param stmt Reference to the SQLite statement to manage. Must remain valid for the lifetime
     * of this wrapper.
     */
    explicit ScopedStatement(Statement& stmt) noexcept : stmt_(&stmt), owns_(true) {}

    /**
     * @brief Constructs a ScopedStatement from a pointer (can be nullptr).
     * @param stmt Pointer to the SQLite statement to manage.
     */
    explicit ScopedStatement(Statement* stmt = nullptr) noexcept
        : stmt_(stmt), owns_(stmt != nullptr) {}

    /**
     * @brief Move constructor.
     */
    ScopedStatement(ScopedStatement&& other) noexcept;

    /**
     * @brief Move assignment.
     */
    ScopedStatement& operator=(ScopedStatement&& other) noexcept;
    // Non‑copyable
    ScopedStatement(const ScopedStatement&) = delete;
    ScopedStatement& operator=(const ScopedStatement&) = delete;

    /**
     * @brief Destructor. Calls reset() if still owning the statement.
     */
    ~ScopedStatement() noexcept;

    /**
     * @brief Explicitly reset the managed statement.
     *
     * Calls reset() on the underlying statement if we own it and the pointer is not null.
     * After this call, ownership is released (the wrapper no longer manages the statement).
     */
    void Reset() noexcept;

    /**
     * @brief Release management of the statement without resetting it.
     * @return Pointer to the released statement (or nullptr).
     *
     * After calling release(), the wrapper no longer owns the statement and will not
     * call reset() in the destructor.
     */
    Statement* Release() noexcept;

    /**
     * @brief Get a pointer to the managed statement.
     */
    Statement* Get() noexcept { return stmt_; }

    /**
     * @brief Get a const pointer to the managed statement.
     */
    const Statement* Get() const noexcept { return stmt_; }

    /**
     * @brief Arrow operator for direct access to Statement methods.
     */
    Statement* operator->() noexcept { return stmt_; }

    /**
     * @brief Const arrow operator.
     */
    const Statement* operator->() const noexcept { return stmt_; }

    /**
     * @brief Dereference operator.
     */
    Statement& operator*() noexcept { return *stmt_; }

    /**
     * @brief Const dereference operator.
     */
    const Statement& operator*() const noexcept { return *stmt_; }

    /**
     * @brief Check if the wrapper currently owns the statement (i.e., will reset it on
     * destruction).
     */
    bool Owns() const noexcept { return owns_; }

    /**
     * @brief Boolean conversion for easy null‑check.
     */
    explicit operator bool() const noexcept { return stmt_ != nullptr; }

 private:
    Statement* stmt_;
    bool owns_;
};

}  // namespace database