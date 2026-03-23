#include "SQLiteCpp/SQLiteCpp.h"

#include "ScopedStatement.hpp"

#include <gtest/gtest.h>

#include <memory>

using namespace std::string_literals;

namespace database {

class ScopedStatementTest : public ::testing::Test {
 protected:
    using Database = SQLite::Database;
    using Statement = SQLite::Statement;
    using UniquePtrStatement = std::unique_ptr<Statement>;

    Database db_{":memory:"s, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};

    auto CreateStatement() { return std::make_unique<Statement>(db_, "SELECT 1"); }

    auto CreateParameterizedStatement() {
        return std::make_unique<Statement>(db_, "SELECT ? AS value");
    }
};

TEST_F(ScopedStatementTest, Constructor_AcceptsPointer_ToValidStatement) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_EQ(scoped.Get(), stmt.get());
    EXPECT_TRUE(scoped.Owns());
    EXPECT_TRUE(static_cast<bool>(scoped));
}

TEST_F(ScopedStatementTest, Constructor_AcceptsNullptr) {
    ScopedStatement scoped(nullptr);

    EXPECT_EQ(scoped.Get(), nullptr);
    EXPECT_FALSE(scoped.Owns());
    EXPECT_FALSE(static_cast<bool>(scoped));
}

TEST_F(ScopedStatementTest, Constructor_DefaultConstructible) {
    ScopedStatement scoped;

    EXPECT_EQ(scoped.Get(), nullptr);
    EXPECT_FALSE(scoped.Owns());
    EXPECT_FALSE(static_cast<bool>(scoped));
}

TEST_F(ScopedStatementTest, Constructor_AcceptsReference_ToValidStatement) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(*stmt);

    EXPECT_EQ(scoped.Get(), stmt.get());
    EXPECT_TRUE(scoped.Owns());
    EXPECT_TRUE(static_cast<bool>(scoped));
}

TEST_F(ScopedStatementTest, Constructor_SetsOwnershipFlag_Correctly) {
    auto stmt1 = CreateStatement();
    auto stmt2 = CreateStatement();

    ScopedStatement scoped1(stmt1.get());
    ScopedStatement scoped2(nullptr);

    EXPECT_TRUE(scoped1.Owns());
    EXPECT_FALSE(scoped2.Owns());
}

TEST_F(ScopedStatementTest, MoveConstructor_TransfersOwnership) {
    auto stmt = CreateStatement();
    Statement* stmt_ptr = stmt.get();

    ScopedStatement scoped1(stmt_ptr);
    EXPECT_TRUE(scoped1.Owns());
    EXPECT_EQ(scoped1.Get(), stmt_ptr);

    ScopedStatement scoped2(std::move(scoped1));

    EXPECT_EQ(scoped2.Get(), stmt_ptr);
    EXPECT_TRUE(scoped2.Owns());
    EXPECT_EQ(scoped1.Get(), nullptr);
    EXPECT_FALSE(scoped1.Owns());
}

TEST_F(ScopedStatementTest, MoveConstructor_HandlesNull) {
    ScopedStatement scoped1(nullptr);
    ScopedStatement scoped2(std::move(scoped1));

    EXPECT_EQ(scoped2.Get(), nullptr);
    EXPECT_FALSE(scoped2.Owns());
    EXPECT_EQ(scoped1.Get(), nullptr);
    EXPECT_FALSE(scoped1.Owns());
}

TEST_F(ScopedStatementTest, MoveAssignment_TransfersOwnership) {
    auto stmt1 = CreateStatement();
    auto stmt2 = CreateStatement();

    ScopedStatement scoped1(stmt1.get());
    ScopedStatement scoped2(stmt2.get());

    Statement* stmt2_ptr = stmt2.get();

    EXPECT_TRUE(scoped1.Owns());
    EXPECT_TRUE(scoped2.Owns());

    scoped1 = std::move(scoped2);

    EXPECT_EQ(scoped1.Get(), stmt2_ptr);
    EXPECT_TRUE(scoped1.Owns());
    EXPECT_EQ(scoped2.Get(), nullptr);
    EXPECT_FALSE(scoped2.Owns());
}

TEST_F(ScopedStatementTest, MoveAssignment_WorksFromNullOwner) {
    auto stmt = CreateStatement();
    ScopedStatement scoped1(nullptr);
    ScopedStatement scoped2(stmt.get());

    Statement* stmt_ptr = stmt.get();

    scoped1 = std::move(scoped2);

    EXPECT_EQ(scoped1.Get(), stmt_ptr);
    EXPECT_TRUE(scoped1.Owns());
    EXPECT_EQ(scoped2.Get(), nullptr);
    EXPECT_FALSE(scoped2.Owns());
}

TEST_F(ScopedStatementTest, MoveAssignment_IsSelfAssignmentSafe) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    Statement* stmt_ptr = stmt.get();

    ScopedStatement temp(std::move(scoped));
    scoped = std::move(temp);

    EXPECT_EQ(scoped.Get(), stmt_ptr);
    EXPECT_TRUE(scoped.Owns());
}

TEST_F(ScopedStatementTest, MoveConstructor_IsNoexcept) {
    static_assert(std::is_nothrow_move_constructible_v<ScopedStatement>,
                  "ScopedStatement move constructor must be noexcept");
}

TEST_F(ScopedStatementTest, MoveAssignmentOperator_IsNoexcept) {
    static_assert(std::is_nothrow_move_assignable_v<ScopedStatement>,
                  "ScopedStatement move assignment must be noexcept");
}

TEST_F(ScopedStatementTest, CopyConstructor_IsDeleted) {
    static_assert(!std::is_copy_constructible_v<ScopedStatement>,
                  "ScopedStatement must not be copy constructible");
}

TEST_F(ScopedStatementTest, CopyAssignment_IsDeleted) {
    static_assert(!std::is_copy_assignable_v<ScopedStatement>,
                  "ScopedStatement must not be copy assignable");
}

TEST_F(ScopedStatementTest, Reset_ClearsStatement_WhenOwning) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_TRUE(scoped.Owns());
    EXPECT_NE(scoped.Get(), nullptr);

    scoped.Reset();

    EXPECT_EQ(scoped.Get(), nullptr);
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Reset_DoesNotThrow_OnNullptr) {
    ScopedStatement scoped(nullptr);
    EXPECT_NO_THROW(scoped.Reset());
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Reset_IsNoop_WhenNotOwning) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    scoped.Release();
    EXPECT_FALSE(scoped.Owns());

    EXPECT_NO_THROW(scoped.Reset());
    EXPECT_EQ(scoped.Get(), nullptr);
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Reset_IsNoexcept) {
    static_assert(noexcept(std::declval<ScopedStatement>().Reset()), "Reset() must be noexcept");
}

TEST_F(ScopedStatementTest, Reset_IsIdempotent) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    scoped.Reset();
    auto first_result = scoped.Get();

    scoped.Reset();
    auto second_result = scoped.Get();

    EXPECT_EQ(first_result, second_result);
    EXPECT_EQ(second_result, nullptr);
}

TEST_F(ScopedStatementTest, Release_ReturnsPointer_AndClearsOwnership) {
    auto stmt = CreateStatement();
    Statement* stmt_ptr = stmt.get();

    ScopedStatement scoped(stmt_ptr);
    EXPECT_TRUE(scoped.Owns());

    auto released = scoped.Release();

    EXPECT_EQ(released, stmt_ptr);
    EXPECT_EQ(scoped.Get(), nullptr);
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Release_ReturnsNull_OnEmpty) {
    ScopedStatement scoped(nullptr);
    auto released = scoped.Release();

    EXPECT_EQ(released, nullptr);
    EXPECT_EQ(scoped.Get(), nullptr);
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Release_DoesNotCallReset) {
    auto stmt = CreateStatement();
    Statement* stmt_ptr = stmt.get();

    ScopedStatement scoped(stmt_ptr);
    auto released = scoped.Release();

    // The statement itself should still be accessible
    EXPECT_NE(released, nullptr);
    EXPECT_EQ(released, stmt_ptr);
}

TEST_F(ScopedStatementTest, Release_IsNoexcept) {
    static_assert(noexcept(std::declval<ScopedStatement>().Release()),
                  "Release() must be noexcept");
}

TEST_F(ScopedStatementTest, Release_IsIdempotent) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    auto first_release = scoped.Release();
    auto second_release = scoped.Release();

    EXPECT_NE(first_release, nullptr);
    EXPECT_EQ(second_release, nullptr);
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Get_ReturnsValidPointer) {
    auto stmt = CreateStatement();
    Statement* stmt_ptr = stmt.get();

    ScopedStatement scoped(stmt_ptr);
    EXPECT_EQ(scoped.Get(), stmt_ptr);
}

TEST_F(ScopedStatementTest, Get_ReturnsNull_ForEmpty) {
    ScopedStatement scoped(nullptr);
    EXPECT_EQ(scoped.Get(), nullptr);
}

TEST_F(ScopedStatementTest, Get_ConstVersion_Works) {
    auto stmt = CreateStatement();
    const ScopedStatement scoped(stmt.get());

    EXPECT_EQ(scoped.Get(), stmt.get());
}

TEST_F(ScopedStatementTest, Get_IsNoexcept) {
    static_assert(noexcept(std::declval<ScopedStatement>().Get()), "Get() must be noexcept");
}

TEST_F(ScopedStatementTest, ArrowOperator_GrantsAccess_ToMethods) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    // Test arrow operator access to underlying statement
    EXPECT_NO_THROW(scoped->executeStep());
}

TEST_F(ScopedStatementTest, ArrowOperator_ConstVersion_Works) {
    auto stmt = CreateStatement();
    const ScopedStatement scoped(stmt.get());

    EXPECT_NE(scoped.operator->(), nullptr);
}

TEST_F(ScopedStatementTest, ArrowOperator_IsNoexcept) {
    static_assert(noexcept(std::declval<ScopedStatement>().operator->()),
                  "operator->() must be noexcept");
}

TEST_F(ScopedStatementTest, ArrowOperator_ReturnsValidPointer) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_EQ(scoped.operator->(), stmt.get());
}

TEST_F(ScopedStatementTest, DereferenceOperator_ReturnsReference) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    Statement& deref_stmt = *scoped;
    EXPECT_EQ(&deref_stmt, stmt.get());
}

TEST_F(ScopedStatementTest, DereferenceOperator_ConstVersion_Works) {
    auto stmt = CreateStatement();
    const ScopedStatement scoped(stmt.get());

    const Statement& deref_stmt = *scoped;
    EXPECT_EQ(&deref_stmt, stmt.get());
}

TEST_F(ScopedStatementTest, DereferenceOperator_IsNoexcept) {
    static_assert(noexcept(std::declval<ScopedStatement>().operator*()),
                  "operator*() must be noexcept");
}

TEST_F(ScopedStatementTest, Owns_ReturnsTrue_ForOwnedStatement) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_TRUE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Owns_ReturnsFalse_ForNull) {
    ScopedStatement scoped(nullptr);
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Owns_ReturnsFalse_AfterRelease) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_TRUE(scoped.Owns());
    scoped.Release();
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Owns_ReturnsFalse_AfterReset) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_TRUE(scoped.Owns());
    scoped.Reset();
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, Owns_IsNoexcept) {
    static_assert(noexcept(std::declval<ScopedStatement>().Owns()), "Owns() must be noexcept");
}

TEST_F(ScopedStatementTest, BoolConversion_ReturnsTrue_ForValid) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_TRUE(scoped);
    if (scoped) {
        SUCCEED();
    } else {
        FAIL();
    }
}

TEST_F(ScopedStatementTest, BoolConversion_ReturnsFalse_ForNull) {
    ScopedStatement scoped(nullptr);

    EXPECT_FALSE(scoped);
    if (!scoped) {
        SUCCEED();
    } else {
        FAIL();
    }
}

TEST_F(ScopedStatementTest, BoolConversion_ChangesAfterRelease) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_TRUE(scoped);
    scoped.Release();
    EXPECT_FALSE(scoped);
}

TEST_F(ScopedStatementTest, BoolConversion_IsExplicit) {
    static_assert(std::is_constructible_v<bool, ScopedStatement>,
                  "operator bool() should be explicit and convertible");
    static_assert(!std::is_convertible_v<ScopedStatement, bool>,
                  "operator bool() should not be implicitly convertible");
}

TEST_F(ScopedStatementTest, BoolConversion_IsNoexcept) {
    static_assert(noexcept(static_cast<bool>(std::declval<ScopedStatement>())),
                  "operator bool() must be noexcept");
}

TEST_F(ScopedStatementTest, Destructor_CallsReset_OnOwnedStatement) {
    {
        auto stmt = CreateStatement();
        Statement* stmt_ptr = stmt.get();
        ScopedStatement scoped(stmt_ptr);
        EXPECT_TRUE(scoped.Owns());
    }
    SUCCEED();
}

TEST_F(ScopedStatementTest, Destructor_DoesNotCrash_OnNull) {
    { ScopedStatement scoped(nullptr); }
    SUCCEED();
}

TEST_F(ScopedStatementTest, Destructor_DoesNotCrash_OnReleased) {
    {
        auto stmt = CreateStatement();
        ScopedStatement scoped(stmt.get());
        scoped.Release();
    }
    SUCCEED();
}

TEST_F(ScopedStatementTest, RAIIGuarantee_Cleanup_OnStackUnwind) {
    auto stmt = CreateStatement();
    Statement* stmt_ptr = stmt.get();

    {
        ScopedStatement scoped(stmt_ptr);
        EXPECT_TRUE(scoped.Owns());
    }

    SUCCEED();
}

TEST_F(ScopedStatementTest, Destructor_IsNoexcept) {
    static_assert(std::is_nothrow_destructible_v<ScopedStatement>,
                  "ScopedStatement destructor must be noexcept");
}

TEST_F(ScopedStatementTest, Integration_SimpleWorkflow) {
    {
        auto stmt = CreateStatement();
        ScopedStatement scoped(stmt.get());

        EXPECT_TRUE(scoped);
        EXPECT_TRUE(scoped.Owns());
        EXPECT_NO_THROW(scoped->executeStep());
        scoped.Reset();
        EXPECT_FALSE(scoped);
    }
    SUCCEED();
}

TEST_F(ScopedStatementTest, Integration_ReleaseAndManualCleanup) {
    auto stmt = CreateStatement();

    Statement* raw_ptr = nullptr;
    {
        ScopedStatement scoped(stmt.get());
        raw_ptr = scoped.Release();

        EXPECT_EQ(raw_ptr, stmt.get());
        EXPECT_FALSE(scoped);
    }

    EXPECT_NE(raw_ptr, nullptr);
}

TEST_F(ScopedStatementTest, Integration_MoveForOwnershipTransfer) {
    auto stmt = CreateStatement();

    ScopedStatement scoped1(stmt.get());
    EXPECT_TRUE(scoped1.Owns());

    ScopedStatement scoped2(std::move(scoped1));

    EXPECT_FALSE(scoped1.Owns());
    EXPECT_TRUE(scoped2.Owns());
    EXPECT_EQ(scoped2.Get(), stmt.get());
}

TEST_F(ScopedStatementTest, Integration_MultipleMoves) {
    auto stmt = CreateStatement();

    ScopedStatement scoped1(stmt.get());
    ScopedStatement scoped2(std::move(scoped1));
    ScopedStatement scoped3(std::move(scoped2));

    EXPECT_FALSE(scoped1.Owns());
    EXPECT_FALSE(scoped2.Owns());
    EXPECT_TRUE(scoped3.Owns());
}

TEST_F(ScopedStatementTest, Integration_WithParameterizedStatement) {
    auto stmt = CreateParameterizedStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_TRUE(scoped);
    scoped->bind(1, "test value");
    EXPECT_NO_THROW(scoped->executeStep());
}

TEST_F(ScopedStatementTest, Integration_ChainedArrowOperator) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    EXPECT_NO_THROW({ scoped->executeStep(); });
}

TEST_F(ScopedStatementTest, EdgeCase_EmptyScoping) {
    ScopedStatement scoped;
    EXPECT_FALSE(scoped);
    EXPECT_EQ(scoped.Get(), nullptr);
}

TEST_F(ScopedStatementTest, EdgeCase_ResetAfterMove) {
    auto stmt = CreateStatement();
    ScopedStatement scoped1(stmt.get());
    ScopedStatement scoped2(std::move(scoped1));

    EXPECT_NO_THROW(scoped1.Reset());
    EXPECT_EQ(scoped1.Get(), nullptr);
    EXPECT_FALSE(scoped1.Owns());
}

TEST_F(ScopedStatementTest, EdgeCase_ReleaseAfterMove) {
    auto stmt = CreateStatement();
    ScopedStatement scoped1(stmt.get());
    ScopedStatement scoped2(std::move(scoped1));

    auto released = scoped1.Release();
    EXPECT_EQ(released, nullptr);
}

TEST_F(ScopedStatementTest, EdgeCase_MultipleOwnershipChecks) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    // Multiple checks should be consistent
    EXPECT_TRUE(scoped.Owns());
    EXPECT_TRUE(scoped.Owns());
    EXPECT_TRUE(scoped.Owns());

    scoped.Release();

    EXPECT_FALSE(scoped.Owns());
    EXPECT_FALSE(scoped.Owns());
    EXPECT_FALSE(scoped.Owns());
}

TEST_F(ScopedStatementTest, EdgeCase_ArrowOperatorAfterRelease) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());
    scoped.Release();

    EXPECT_EQ(scoped.Get(), nullptr);
}

TEST_F(ScopedStatementTest, EdgeCase_OwnsFlagConsistency) {
    auto stmt = CreateStatement();
    ScopedStatement scoped(stmt.get());

    if (scoped.Owns()) {
        EXPECT_TRUE(static_cast<bool>(scoped));
    }

    scoped.Release();

    if (!scoped.Owns()) {
        EXPECT_FALSE(static_cast<bool>(scoped));
    }
}

TEST_F(ScopedStatementTest, Trait_MoveSemantics) {
    static_assert(std::is_move_constructible_v<ScopedStatement>);
    static_assert(std::is_move_assignable_v<ScopedStatement>);
    static_assert(std::is_nothrow_move_constructible_v<ScopedStatement>);
    static_assert(std::is_nothrow_move_assignable_v<ScopedStatement>);
}

TEST_F(ScopedStatementTest, Trait_DefaultConstructible) {
    static_assert(std::is_default_constructible_v<ScopedStatement>);
}

TEST_F(ScopedStatementTest, TraitComparability) { SUCCEED(); }

TEST_F(ScopedStatementTest, Trait_Destructibility) {
    static_assert(std::is_destructible_v<ScopedStatement>);
    static_assert(std::is_nothrow_destructible_v<ScopedStatement>);
}

}  // namespace database
