#include "gtest/gtest.h"

#include "TaskDB.hpp"

#include <string>
#include <string_view>

using namespace database;
using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

namespace {

class TaskDBTest : public ::testing::Test {
 protected:
    static constexpr int64_t kUserId = 1001;
    static constexpr int64_t kOtherUserId = 2002;
    static constexpr std::string_view kDefaultUserName = "alice"sv;

    TaskDB db;

    void AddDefaultUser() { db.AddUser(kUserId, std::string(kDefaultUserName)); }

    int64_t AddTaskForDefaultUser(std::string_view text,
                                  TaskStatus status = TaskStatus::ACTIVE) {
        const auto id = db.AddTask(kUserId, std::string(text), status);
        EXPECT_TRUE(id.has_value());
        return id.value_or(-1);
    }
};

TEST_F(TaskDBTest, IsConnected_Returns_True_After_Construction) {
    EXPECT_TRUE(db.IsConnected());
}

TEST_F(TaskDBTest, IsUserExist_Returns_False_For_Unknown_User) {
    EXPECT_FALSE(db.IsUserExist(kUserId));
}

TEST_F(TaskDBTest, AddUser_Makes_User_Visible_In_Database) {
    AddDefaultUser();
    EXPECT_TRUE(db.IsUserExist(kUserId));
}

TEST_F(TaskDBTest, AddUser_Throws_For_Duplicate_User_Id) {
    AddDefaultUser();
    EXPECT_THROW(db.AddUser(kUserId, "alice-duplicate"s), std::runtime_error);
}

TEST_F(TaskDBTest, AddTask_Throws_When_Owner_User_Does_Not_Exist) {
    EXPECT_THROW(static_cast<void>(db.AddTask(kUserId, "task without owner"s)),
                 std::runtime_error);
}

TEST_F(TaskDBTest, AddTask_Returns_Created_Task_Id) {
    AddDefaultUser();
    const auto first_task_id = db.AddTask(kUserId, "first"s);
    const auto second_task_id = db.AddTask(kUserId, "second"s);

    ASSERT_TRUE(first_task_id.has_value());
    ASSERT_TRUE(second_task_id.has_value());
    EXPECT_LT(*first_task_id, *second_task_id);
}

TEST_F(TaskDBTest, GetAllTasks_Returns_Nullopt_For_User_Without_Tasks) {
    AddDefaultUser();
    EXPECT_FALSE(db.GetAllTasks(kUserId).has_value());
}

TEST_F(TaskDBTest, GetAllTasks_Returns_Tasks_In_Insertion_Order) {
    AddDefaultUser();
    AddTaskForDefaultUser("task-1"sv, TaskStatus::ACTIVE);
    AddTaskForDefaultUser("task-2"sv, TaskStatus::COMPLETED);
    AddTaskForDefaultUser("task-3"sv, TaskStatus::ACTIVE);

    const auto tasks = db.GetAllTasks(kUserId);
    ASSERT_TRUE(tasks.has_value());
    ASSERT_EQ(tasks->size(), 3U);

    EXPECT_EQ((*tasks)[0].text, "task-1"s);
    EXPECT_EQ((*tasks)[0].status, TaskStatus::ACTIVE);
    EXPECT_EQ((*tasks)[1].text, "task-2"s);
    EXPECT_EQ((*tasks)[1].status, TaskStatus::COMPLETED);
    EXPECT_EQ((*tasks)[2].text, "task-3"s);
    EXPECT_EQ((*tasks)[2].status, TaskStatus::ACTIVE);
}

TEST_F(TaskDBTest, GetActiveOrCompletedTasks_Filters_By_Status) {
    AddDefaultUser();
    AddTaskForDefaultUser("active-1"sv, TaskStatus::ACTIVE);
    AddTaskForDefaultUser("completed-1"sv, TaskStatus::COMPLETED);
    AddTaskForDefaultUser("active-2"sv, TaskStatus::ACTIVE);

    const auto active_tasks = db.GetActiveOrCompletedTasks(kUserId, TaskStatus::ACTIVE);
    const auto completed_tasks = db.GetActiveOrCompletedTasks(kUserId, TaskStatus::COMPLETED);

    ASSERT_TRUE(active_tasks.has_value());
    ASSERT_TRUE(completed_tasks.has_value());
    ASSERT_EQ(active_tasks->size(), 2U);
    ASSERT_EQ(completed_tasks->size(), 1U);

    EXPECT_EQ((*active_tasks)[0].status, TaskStatus::ACTIVE);
    EXPECT_EQ((*active_tasks)[1].status, TaskStatus::ACTIVE);
    EXPECT_EQ((*completed_tasks)[0].status, TaskStatus::COMPLETED);
}

TEST_F(TaskDBTest, GetActiveOrCompletedTasks_Returns_Nullopt_If_No_Tasks_For_Status) {
    AddDefaultUser();
    AddTaskForDefaultUser("active-only"sv, TaskStatus::ACTIVE);

    const auto completed_tasks = db.GetActiveOrCompletedTasks(kUserId, TaskStatus::COMPLETED);
    EXPECT_FALSE(completed_tasks.has_value());
}

TEST_F(TaskDBTest, GetUserStatistics_Returns_Zero_For_User_Without_Tasks) {
    AddDefaultUser();
    const TaskStatistics stats = db.GetUserStatistics(kUserId);

    EXPECT_EQ(stats.total, 0);
    EXPECT_EQ(stats.completed, 0);
    EXPECT_EQ(stats.active, 0);
}

TEST_F(TaskDBTest, GetUserStatistics_Returns_Correct_Counts) {
    AddDefaultUser();
    AddTaskForDefaultUser("a1"sv, TaskStatus::ACTIVE);
    AddTaskForDefaultUser("c1"sv, TaskStatus::COMPLETED);
    AddTaskForDefaultUser("a2"sv, TaskStatus::ACTIVE);

    const TaskStatistics stats = db.GetUserStatistics(kUserId);

    EXPECT_EQ(stats.total, 3);
    EXPECT_EQ(stats.completed, 1);
    EXPECT_EQ(stats.active, 2);
}

TEST_F(TaskDBTest, UpdateTaskStatus_Changes_Only_Owner_Task) {
    AddDefaultUser();
    db.AddUser(kOtherUserId, "bob"s);
    const int64_t own_task = AddTaskForDefaultUser("own"sv, TaskStatus::ACTIVE);
    const auto other_task_opt = db.AddTask(kOtherUserId, "other"s, TaskStatus::ACTIVE);
    ASSERT_TRUE(other_task_opt.has_value());

    db.UpdateTaskStatus(kUserId, own_task, TaskStatus::COMPLETED);
    db.UpdateTaskStatus(kUserId, *other_task_opt, TaskStatus::COMPLETED);

    const auto own_tasks = db.GetAllTasks(kUserId);
    const auto other_tasks = db.GetAllTasks(kOtherUserId);

    ASSERT_TRUE(own_tasks.has_value());
    ASSERT_TRUE(other_tasks.has_value());
    ASSERT_EQ(own_tasks->size(), 1U);
    ASSERT_EQ(other_tasks->size(), 1U);

    EXPECT_EQ((*own_tasks)[0].status, TaskStatus::COMPLETED);
    EXPECT_EQ((*other_tasks)[0].status, TaskStatus::ACTIVE);
}

TEST_F(TaskDBTest, UpdateTaskStatus_For_Missing_Task_Does_Not_Throw) {
    AddDefaultUser();
    EXPECT_NO_THROW(db.UpdateTaskStatus(kUserId, 999999, TaskStatus::COMPLETED));
}

TEST_F(TaskDBTest, EditTask_Changes_Task_Text) {
    AddDefaultUser();
    const int64_t task_id = AddTaskForDefaultUser("before"sv, TaskStatus::ACTIVE);

    db.EditTask(kUserId, task_id, "after"s);

    const auto tasks = db.GetAllTasks(kUserId);
    ASSERT_TRUE(tasks.has_value());
    ASSERT_EQ(tasks->size(), 1U);
    EXPECT_EQ((*tasks)[0].text, "after"s);
}

TEST_F(TaskDBTest, EditTask_With_Same_Text_Keeps_Task_And_Does_Not_Throw) {
    AddDefaultUser();
    const int64_t task_id = AddTaskForDefaultUser("unchanged"sv, TaskStatus::ACTIVE);

    EXPECT_NO_THROW(db.EditTask(kUserId, task_id, "unchanged"s));

    const auto tasks = db.GetAllTasks(kUserId);
    ASSERT_TRUE(tasks.has_value());
    ASSERT_EQ(tasks->size(), 1U);
    EXPECT_EQ((*tasks)[0].text, "unchanged"s);
}

TEST_F(TaskDBTest, EditTask_Throws_For_Missing_Task) {
    AddDefaultUser();
    EXPECT_THROW(db.EditTask(kUserId, 123456, "new text"s), std::runtime_error);
}

TEST_F(TaskDBTest, EditTask_Throws_For_Foreign_Task_Ownership) {
    AddDefaultUser();
    db.AddUser(kOtherUserId, "bob"s);
    const auto foreign_task = db.AddTask(kOtherUserId, "bob-task"s, TaskStatus::ACTIVE);
    ASSERT_TRUE(foreign_task.has_value());

    EXPECT_THROW(db.EditTask(kUserId, *foreign_task, "hack"s), std::runtime_error);
}

TEST_F(TaskDBTest, DeleteTask_Removes_Existing_Task) {
    AddDefaultUser();
    const int64_t task_id = AddTaskForDefaultUser("to-delete"sv, TaskStatus::ACTIVE);

    db.DeleteTask(kUserId, task_id);

    const auto tasks = db.GetAllTasks(kUserId);
    EXPECT_FALSE(tasks.has_value());
}

TEST_F(TaskDBTest, DeleteTask_Throws_For_Missing_Task) {
    AddDefaultUser();
    EXPECT_THROW(db.DeleteTask(kUserId, 987654), std::runtime_error);
}

TEST_F(TaskDBTest, DeleteTask_Throws_For_Foreign_Task_Ownership) {
    AddDefaultUser();
    db.AddUser(kOtherUserId, "bob"s);
    const auto foreign_task = db.AddTask(kOtherUserId, "foreign"s, TaskStatus::ACTIVE);
    ASSERT_TRUE(foreign_task.has_value());

    EXPECT_THROW(db.DeleteTask(kUserId, *foreign_task), std::runtime_error);
}

TEST_F(TaskDBTest, DeleteAllUserTasks_Removes_Only_Specified_Users_Tasks) {
    AddDefaultUser();
    db.AddUser(kOtherUserId, "bob"s);
    AddTaskForDefaultUser("a"sv, TaskStatus::ACTIVE);
    AddTaskForDefaultUser("b"sv, TaskStatus::COMPLETED);
    const auto other_task = db.AddTask(kOtherUserId, "bob-task"s, TaskStatus::ACTIVE);
    ASSERT_TRUE(other_task.has_value());

    db.DeleteAllUserTasks(kUserId);

    EXPECT_FALSE(db.GetAllTasks(kUserId).has_value());
    const auto other_tasks = db.GetAllTasks(kOtherUserId);
    ASSERT_TRUE(other_tasks.has_value());
    ASSERT_EQ(other_tasks->size(), 1U);
    EXPECT_EQ((*other_tasks)[0].text, "bob-task"s);
}

TEST_F(TaskDBTest, Begin_CommitTransaction_Persists_Changes) {
    db.BeginTransaction();
    db.AddUser(kUserId, "alice"s);
    const auto task_id = db.AddTask(kUserId, "inside-commit"s, TaskStatus::ACTIVE);
    ASSERT_TRUE(task_id.has_value());
    db.CommitTransaction();

    EXPECT_TRUE(db.IsUserExist(kUserId));
    const auto tasks = db.GetAllTasks(kUserId);
    ASSERT_TRUE(tasks.has_value());
    ASSERT_EQ(tasks->size(), 1U);
    EXPECT_EQ((*tasks)[0].text, "inside-commit"s);
}

TEST_F(TaskDBTest, Begin_RollbackTransaction_Reverts_Changes) {
    db.BeginTransaction();
    db.AddUser(kUserId, "alice"s);
    const auto task_id = db.AddTask(kUserId, "inside-rollback"s, TaskStatus::ACTIVE);
    ASSERT_TRUE(task_id.has_value());
    db.RollbackTransaction();

    EXPECT_FALSE(db.IsUserExist(kUserId));
    EXPECT_FALSE(db.GetAllTasks(kUserId).has_value());
}

}  // namespace
