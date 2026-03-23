#include "gtest/gtest.h"

#include "TaskDB.hpp"

#include <string>

using namespace database;
using std::string_literals::operator""s;

namespace {

class TransactionGuardTest : public ::testing::Test {
 protected:
    static constexpr int64_t kUserId = 777;
    static constexpr int64_t kOtherUserId = 778;

    TaskDB db;
};

TEST_F(TransactionGuardTest, CreateGuard_Commit_Persists_Changes) {
    auto guard = db.CreateTransactionGuard();
    db.AddUser(kUserId, "user"s);
    const auto task_id = db.AddTask(kUserId, "task"s, TaskStatus::ACTIVE);

    ASSERT_TRUE(task_id.has_value());
    guard.Commit();

    EXPECT_TRUE(db.IsUserExist(kUserId));
    const auto tasks = db.GetAllTasks(kUserId);
    ASSERT_TRUE(tasks.has_value());
    ASSERT_EQ(tasks->size(), 1U);
    EXPECT_EQ((*tasks)[0].text, "task"s);
}

TEST_F(TransactionGuardTest, CreateGuard_ExplicitRollback_Reverts_Changes) {
    auto guard = db.CreateTransactionGuard();
    db.AddUser(kUserId, "user"s);
    const auto task_id = db.AddTask(kUserId, "to rollback"s, TaskStatus::ACTIVE);

    ASSERT_TRUE(task_id.has_value());
    guard.Rollback();

    EXPECT_FALSE(db.IsUserExist(kUserId));
    EXPECT_FALSE(db.GetAllTasks(kUserId).has_value());
}

TEST_F(TransactionGuardTest, CreateGuard_DestructorWithoutCommit_RollsBack) {
    {
        auto guard = db.CreateTransactionGuard();
        db.AddUser(kUserId, "user"s);
        const auto task_id = db.AddTask(kUserId, "auto rollback"s, TaskStatus::ACTIVE);
        ASSERT_TRUE(task_id.has_value());
        EXPECT_TRUE(guard.IsActive());
    }

    EXPECT_FALSE(db.IsUserExist(kUserId));
    EXPECT_FALSE(db.GetAllTasks(kUserId).has_value());
}

TEST_F(TransactionGuardTest, CreateGuard_DoubleCommit_ThrowsLogicError) {
    auto guard = db.CreateTransactionGuard();
    guard.Commit();

    EXPECT_THROW(guard.Commit(), std::logic_error);
}

TEST_F(TransactionGuardTest, CreateGuard_MoveCtor_TransfersOwnership) {
    auto first_guard = db.CreateTransactionGuard();
    auto second_guard = std::move(first_guard);

    EXPECT_FALSE(first_guard.IsActive());
    EXPECT_TRUE(second_guard.IsActive());

    db.AddUser(kUserId, "user"s);
    second_guard.Commit();
    EXPECT_TRUE(db.IsUserExist(kUserId));
}

TEST_F(TransactionGuardTest, CreateGuard_MoveAssign_TransfersOwnership) {
    TaskDB other_db;

    auto first_guard = db.CreateTransactionGuard();
    auto second_guard = other_db.CreateTransactionGuard();

    db.AddUser(kUserId, "user"s);
    other_db.AddUser(kOtherUserId, "other user"s);

    second_guard = std::move(first_guard);

    EXPECT_FALSE(first_guard.IsActive());
    EXPECT_TRUE(second_guard.IsActive());

    second_guard.Commit();

    EXPECT_TRUE(db.IsUserExist(kUserId));
    EXPECT_FALSE(other_db.IsUserExist(kOtherUserId));
}

}  // namespace
