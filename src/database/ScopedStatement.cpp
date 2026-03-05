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
        stmt_->reset();
    }
    owns_ = false;
}

Statement* ScopedStatement::Release() noexcept {
    owns_ = false;
    return stmt_;
}

}  // namespace database
