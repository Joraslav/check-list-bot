#include "StatementManager.hpp"

#include "Log.hpp"
#include "magic_enum/magic_enum.hpp"
#include "SQL.hpp"
#include "Utils.hpp"

#include <format>

namespace database {

using std::string_literals::operator""s;

StatementManager::StatementManager(StatementManager::Database& db) : db_(db) {
    try {
        PrepareStatements();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("Failed to prepare statements: {}", e.what()));
    }
}

void StatementManager::PrepareStatements() {
    using namespace utils;
    const auto statements_types = EnumRange(StatementType::COUNT);
    const std::array<std::string_view, 12> exprasions = SQL::GetPreparedStatements();
    for (const auto& [type, expr] : std::ranges::zip_view(statements_types, exprasions)) {
        statements_[type] = std::make_unique<Statement>(db_, std::string(expr));
    }
}

StatementManager::Statement& StatementManager::Get(StatementType type) {
    if (!statements_.contains(type)) {
        throw std::out_of_range(std::format("Statement {} not found", magic_enum::enum_name(type)));
    }
    return *statements_[type];
}

const StatementManager::Statement& StatementManager::Get(StatementType type) const {
    auto it = statements_.find(type);
    if (it == statements_.end()) {
        throw std::out_of_range(std::format("Statement {} not found", magic_enum::enum_name(type)));
    }
    return *it->second;
}

void StatementManager::ResetAll() {
    for (auto& statement_node : statements_) {
        if (statement_node.second) {
            try {
                statement_node.second->reset();
            } catch (const StatementManager::Exception& e) {
                throw std::runtime_error(
                    std::format("Failed to reset statement with type: {}, Error: {}",
                                magic_enum::enum_name(statement_node.first), e.what()));
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to reset. Unknown error: "s.append(e.what()));
            }
        }
    }
}

}  // namespace database
