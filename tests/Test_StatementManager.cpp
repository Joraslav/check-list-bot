#include "gtest/gtest.h"

#include "StatementManager.hpp"
#include "Task.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

using namespace database;
using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

namespace {

int OpenFlags() noexcept { return SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE; }

class StatementManagerTest : public ::testing::Test {
 protected:
    using Database = SQLite::Database;
    using Statement = SQLite::Statement;

    Database db{":memory:", OpenFlags()};
    std::unique_ptr<StatementManager> manager;

    void SetUp() override {
        InitializeSchema(db);
        manager = std::make_unique<StatementManager>(db);
    }

    static void InitializeSchema(Database& db_ref) {
        db_ref.exec("PRAGMA foreign_keys = ON"s);
        db_ref.exec(R"(
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
		)");
    }

    void InsertUserRaw(int64_t user_id, std::string_view user_name) {
        Statement stmt(db, "INSERT INTO users (user_id, user_name) VALUES (?, ?)");
        stmt.bind(1, user_id);
        stmt.bind(2, std::string(user_name));
        stmt.exec();
    }

    int64_t InsertTaskRaw(int64_t user_id, std::string_view text, TaskStatus status) {
        Statement stmt(db,
                       "INSERT INTO tasks (user_id, text, status) VALUES (?, ?, ?) RETURNING id");
        stmt.bind(1, user_id);
        stmt.bind(2, std::string(text));
        stmt.bind(3, static_cast<int>(status));
        const bool has_row = stmt.executeStep();
        EXPECT_TRUE(has_row);
        return has_row ? stmt.getColumn(0).getInt64() : -1;
    }

    [[nodiscard]] int CountRows(std::string_view table_name) const {
        Statement stmt(db, "SELECT COUNT(*) FROM "s.append(std::string(table_name)));
        const bool has_row = stmt.executeStep();
        EXPECT_TRUE(has_row);
        return has_row ? stmt.getColumn(0).getInt() : -1;
    }

    [[nodiscard]] int TaskStatusById(int64_t task_id) const {
        Statement stmt(db, "SELECT status FROM tasks WHERE id = ?");
        stmt.bind(1, task_id);
        const bool has_row = stmt.executeStep();
        EXPECT_TRUE(has_row);
        return has_row ? stmt.getColumn(0).getInt() : -1;
    }

    [[nodiscard]] std::string TaskTextById(int64_t task_id) const {
        Statement stmt(db, "SELECT text FROM tasks WHERE id = ?");
        stmt.bind(1, task_id);
        const bool has_row = stmt.executeStep();
        EXPECT_TRUE(has_row);
        return has_row ? stmt.getColumn(0).getString() : ""s;
    }
};

TEST(StatementManagerCtorTest, Constructor_Throws_When_Schema_Is_Missing) {
    SQLite::Database empty_db(":memory:", OpenFlags());
    EXPECT_THROW(static_cast<void>(StatementManager(empty_db)), std::runtime_error);
}

TEST_F(StatementManagerTest, Constructor_Prepares_All_Statements) {
    for (int index = 0; index < static_cast<int>(StatementType::COUNT); ++index) {
        const auto type = static_cast<StatementType>(index);
        EXPECT_NO_THROW(static_cast<void>(manager->Get(type)));
        const StatementManager& const_manager = *manager;
        EXPECT_NO_THROW(static_cast<void>(const_manager.Get(type)));
    }
}

TEST_F(StatementManagerTest, Get_Returns_Stable_Reference_For_Const_And_NonConst) {
    auto& non_const_stmt_first = manager->Get(StatementType::INSERT_USER);
    auto& non_const_stmt_second = manager->Get(StatementType::INSERT_USER);

    const StatementManager& const_manager = *manager;
    const auto& const_stmt = const_manager.Get(StatementType::INSERT_USER);

    EXPECT_EQ(&non_const_stmt_first, &non_const_stmt_second);
    EXPECT_EQ(&non_const_stmt_first, &const_stmt);
}

TEST_F(StatementManagerTest, Get_Throws_For_Invalid_Statement_Type) {
    const auto invalid_type =
        static_cast<StatementType>(static_cast<int>(StatementType::COUNT) + 1);

    EXPECT_THROW(static_cast<void>(manager->Get(invalid_type)), std::out_of_range);

    const StatementManager& const_manager = *manager;
    EXPECT_THROW(static_cast<void>(const_manager.Get(invalid_type)), std::out_of_range);
}

TEST_F(StatementManagerTest, ResetAll_Resets_Busy_Statement_And_Allows_Reuse) {
    InsertUserRaw(1001, "alice"sv);

    auto& select_user_stmt = manager->Get(StatementType::SELECT_USER);
    select_user_stmt.bind(1, 1001);
    ASSERT_TRUE(select_user_stmt.executeStep());

    EXPECT_NO_THROW(manager->ResetAll());

    select_user_stmt.bind(1, 9999);
    EXPECT_FALSE(select_user_stmt.executeStep());
}

TEST_F(StatementManagerTest, InsertUser_And_SelectUser_Statements_Work) {
    auto& insert_user_stmt = manager->Get(StatementType::INSERT_USER);
    insert_user_stmt.bind(1, 1001);
    insert_user_stmt.bindNoCopy(2, "alice");
    insert_user_stmt.exec();

    auto& user_exist_stmt = manager->Get(StatementType::SELECT_IS_USER_EXIST);
    user_exist_stmt.bind(1, 1001);
    EXPECT_TRUE(user_exist_stmt.executeStep());

    auto& select_user_stmt = manager->Get(StatementType::SELECT_USER);
    select_user_stmt.bind(1, 1001);
    ASSERT_TRUE(select_user_stmt.executeStep());
    EXPECT_GT(select_user_stmt.getColumn(0).getInt64(), 0);
}

TEST_F(StatementManagerTest, InsertTask_And_SelectAllTasks_Work_And_Keep_Order_By_Id) {
    InsertUserRaw(1001, "alice"sv);

    auto& insert_task_stmt = manager->Get(StatementType::INSERT_TASK);

    insert_task_stmt.bind(1, 1001);
    insert_task_stmt.bindNoCopy(2, "task-1");
    insert_task_stmt.bind(3, static_cast<int>(TaskStatus::ACTIVE));
    ASSERT_TRUE(insert_task_stmt.executeStep());
    const int64_t first_id = insert_task_stmt.getColumn(0).getInt64();

    manager->ResetAll();

    insert_task_stmt.bind(1, 1001);
    insert_task_stmt.bindNoCopy(2, "task-2");
    insert_task_stmt.bind(3, static_cast<int>(TaskStatus::COMPLETED));
    ASSERT_TRUE(insert_task_stmt.executeStep());
    const int64_t second_id = insert_task_stmt.getColumn(0).getInt64();

    EXPECT_LT(first_id, second_id);

    manager->ResetAll();

    auto& select_all_stmt = manager->Get(StatementType::SELECT_TASKS_ALL);
    select_all_stmt.bind(1, 1001);

    ASSERT_TRUE(select_all_stmt.executeStep());
    EXPECT_EQ(select_all_stmt.getColumn(0).getInt64(), first_id);
    EXPECT_EQ(select_all_stmt.getColumn(1).getString(), "task-1"sv);
    EXPECT_EQ(select_all_stmt.getColumn(2).getInt(), static_cast<int>(TaskStatus::ACTIVE));

    ASSERT_TRUE(select_all_stmt.executeStep());
    EXPECT_EQ(select_all_stmt.getColumn(0).getInt64(), second_id);
    EXPECT_EQ(select_all_stmt.getColumn(1).getString(), "task-2"sv);
    EXPECT_EQ(select_all_stmt.getColumn(2).getInt(), static_cast<int>(TaskStatus::COMPLETED));

    EXPECT_FALSE(select_all_stmt.executeStep());
}

TEST_F(StatementManagerTest, SelectTasksByStatus_Filters_Rows_Correctly) {
    InsertUserRaw(1001, "alice"sv);
    const auto active_id = InsertTaskRaw(1001, "active"sv, TaskStatus::ACTIVE);
    const auto completed_id = InsertTaskRaw(1001, "completed"sv, TaskStatus::COMPLETED);

    auto& select_by_status_stmt = manager->Get(StatementType::SELECT_TASKS_BY_STATUS);
    select_by_status_stmt.bind(1, 1001);
    select_by_status_stmt.bind(2, static_cast<int>(TaskStatus::COMPLETED));

    ASSERT_TRUE(select_by_status_stmt.executeStep());
    EXPECT_EQ(select_by_status_stmt.getColumn(0).getInt64(), completed_id);
    EXPECT_EQ(select_by_status_stmt.getColumn(1).getString(), "completed"sv);
    EXPECT_EQ(select_by_status_stmt.getColumn(2).getInt(), static_cast<int>(TaskStatus::COMPLETED));
    EXPECT_FALSE(select_by_status_stmt.executeStep());

    EXPECT_NE(active_id, completed_id);
}

TEST_F(StatementManagerTest, UpdateStatus_Updates_Only_Matching_Task) {
    InsertUserRaw(1001, "alice"sv);
    InsertUserRaw(2002, "bob"sv);
    const int64_t alice_task = InsertTaskRaw(1001, "alice-task"sv, TaskStatus::ACTIVE);
    const int64_t bob_task = InsertTaskRaw(2002, "bob-task"sv, TaskStatus::ACTIVE);

    auto& update_stmt = manager->Get(StatementType::UPDATE_STATUS);
    update_stmt.bind(1, static_cast<int>(TaskStatus::COMPLETED));
    update_stmt.bind(2, 1001);
    update_stmt.bind(3, alice_task);
    update_stmt.exec();

    EXPECT_EQ(TaskStatusById(alice_task), static_cast<int>(TaskStatus::COMPLETED));
    EXPECT_EQ(TaskStatusById(bob_task), static_cast<int>(TaskStatus::ACTIVE));

    manager->ResetAll();

    update_stmt.bind(1, static_cast<int>(TaskStatus::COMPLETED));
    update_stmt.bind(2, 1001);
    update_stmt.bind(3, bob_task);
    update_stmt.exec();

    EXPECT_EQ(TaskStatusById(bob_task), static_cast<int>(TaskStatus::ACTIVE));
}

TEST_F(StatementManagerTest, EditTask_And_DeleteTask_Modify_Data_As_Expected) {
    InsertUserRaw(1001, "alice"sv);
    const int64_t keep_task = InsertTaskRaw(1001, "old-text"sv, TaskStatus::ACTIVE);
    const int64_t delete_task = InsertTaskRaw(1001, "to-delete"sv, TaskStatus::ACTIVE);

    auto& edit_stmt = manager->Get(StatementType::EDIT_TASK);
    edit_stmt.bindNoCopy(1, "new-text");
    edit_stmt.bind(2, 1001);
    edit_stmt.bind(3, keep_task);
    edit_stmt.exec();

    EXPECT_EQ(TaskTextById(keep_task), "new-text"sv);

    auto& delete_stmt = manager->Get(StatementType::DELETE_TASK);
    delete_stmt.bind(1, 1001);
    delete_stmt.bind(2, delete_task);
    delete_stmt.exec();

    EXPECT_EQ(CountRows("tasks"sv), 1);
    EXPECT_EQ(TaskTextById(keep_task), "new-text"sv);
}

TEST_F(StatementManagerTest, DeleteAllUserTasks_Deletes_Only_Target_User_Rows) {
    InsertUserRaw(1001, "alice"sv);
    InsertUserRaw(2002, "bob"sv);
    InsertTaskRaw(1001, "a-1"sv, TaskStatus::ACTIVE);
    InsertTaskRaw(1001, "a-2"sv, TaskStatus::COMPLETED);
    InsertTaskRaw(2002, "b-1"sv, TaskStatus::ACTIVE);

    auto& delete_all_stmt = manager->Get(StatementType::DELETE_ALL_USER_TASKS);
    delete_all_stmt.bind(1, 1001);
    delete_all_stmt.exec();

    SQLite::Statement count_alice(db, "SELECT COUNT(*) FROM tasks WHERE user_id = 1001");
    ASSERT_TRUE(count_alice.executeStep());
    EXPECT_EQ(count_alice.getColumn(0).getInt(), 0);

    SQLite::Statement count_bob(db, "SELECT COUNT(*) FROM tasks WHERE user_id = 2002");
    ASSERT_TRUE(count_bob.executeStep());
    EXPECT_EQ(count_bob.getColumn(0).getInt(), 1);
}

TEST_F(StatementManagerTest, VerifyOwnership_And_UserStatistics_Return_Correct_Data) {
    InsertUserRaw(1001, "alice"sv);
    InsertUserRaw(2002, "bob"sv);
    const int64_t alice_active = InsertTaskRaw(1001, "a-active"sv, TaskStatus::ACTIVE);
    static_cast<void>(InsertTaskRaw(1001, "a-completed"sv, TaskStatus::COMPLETED));
    const int64_t bob_active = InsertTaskRaw(2002, "b-active"sv, TaskStatus::ACTIVE);

    auto& verify_stmt = manager->Get(StatementType::VERIFY_TASK_OWNERSHIP);
    verify_stmt.bind(1, alice_active);
    verify_stmt.bind(2, 1001);
    EXPECT_TRUE(verify_stmt.executeStep());

    manager->ResetAll();

    verify_stmt.bind(1, bob_active);
    verify_stmt.bind(2, 1001);
    EXPECT_FALSE(verify_stmt.executeStep());

    auto& stats_stmt = manager->Get(StatementType::GET_USER_STATISTICS);
    stats_stmt.bind(1, 1001);
    ASSERT_TRUE(stats_stmt.executeStep());
    EXPECT_EQ(stats_stmt.getColumn(0).getInt(), 2);
    EXPECT_EQ(stats_stmt.getColumn(1).getInt(), 1);
    EXPECT_EQ(stats_stmt.getColumn(2).getInt(), 1);
}

}  // namespace
