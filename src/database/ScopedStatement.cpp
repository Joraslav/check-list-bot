#include "ScopedStatement.hpp"

using SQLite::Statement;

namespace database {

ScopedStatement::~ScopedStatement() noexcept { Reset(); }

ScopedStatement::ScopedStatement(ScopedStatement&& other) noexcept
    : stmt_(other.stmt_), owns_(other.owns_) {
    other.Release();
}

ScopedStatement& ScopedStatement::operator=(ScopedStatement&& other) noexcept {
    if (this != &other) {
        Reset();
        stmt_ = other.stmt_;
        owns_ = other.owns_;
        other.Release();
    }
    return *this;
}

void ScopedStatement::Reset() noexcept {
    if (owns_ && stmt_ != nullptr) {
        try {
            stmt_->reset();
        } catch (...) {
            // Intentionally ignore exceptions: cleanup code in a noexcept function
            // cannot propagate exceptions, so we continue with cleanup anyway.
            constexpr bool ignored_exception = true;
            static_cast<void>(ignored_exception);
        }
    }
    owns_ = false;
    stmt_ = nullptr;
}

Statement* ScopedStatement::Release() noexcept {
    Statement* released_stmt = stmt_;
    owns_ = false;
    stmt_ = nullptr;
    return released_stmt;
}

}  // namespace database
