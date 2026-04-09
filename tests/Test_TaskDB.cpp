#include "gtest/gtest.h"

#include "TaskDB.hpp"

#include <atomic>
#include <format>
#include <latch>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace database;
using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

namespace {

class TaskDBTest : public ::testing::Test {
 protected:
    static constexpr int64_t kUserId = 1001;
    static constexpr int64_t kOtherUserId = 2002;
    static constexpr std::string_view kDefaultUserName = "alice"sv;

    TaskDB db;  // NOLINT(misc-non-private-member-variables-in-classes)

    void AddDefaultUser() { db.AddUser(kUserId, std::string(kDefaultUserName)); }

    int64_t AddTaskForUser(int64_t user_id, std::string_view text,
                           TaskStatus status = TaskStatus::ACTIVE) {
        db.BeginTransaction();
        try {
            const auto id = db.AddTask(user_id, std::string(text), status);
            EXPECT_TRUE(id.has_value());
            db.CommitTransaction();
            return id.value_or(-1);
        } catch (...) {
            try {
                db.RollbackTransaction();
            } catch (...) {
                ADD_FAILURE()
                    << "RollbackTransaction failed while handling AddTaskForUser exception";
            }
            throw;
        }
    }

    int64_t AddTaskForDefaultUser(std::string_view text, TaskStatus status = TaskStatus::ACTIVE) {
        return AddTaskForUser(kUserId, text, status);
    }
};

TEST_F(TaskDBTest, IsConnected_Returns_True_After_Construction) { EXPECT_TRUE(db.IsConnected()); }

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
    EXPECT_THROW(static_cast<void>(db.AddTask(kUserId, "task without owner"s)), std::runtime_error);
}

TEST_F(TaskDBTest, AddTask_Returns_Created_Task_Id) {
    AddDefaultUser();
    const int64_t first_task_id = AddTaskForDefaultUser("first"sv, TaskStatus::ACTIVE);
    const int64_t second_task_id = AddTaskForDefaultUser("second"sv, TaskStatus::ACTIVE);

    EXPECT_LT(first_task_id, second_task_id);
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
    const int64_t other_task = AddTaskForUser(kOtherUserId, "other"sv, TaskStatus::ACTIVE);

    db.UpdateTaskStatus(kUserId, own_task, TaskStatus::COMPLETED);
    db.UpdateTaskStatus(kUserId, other_task, TaskStatus::COMPLETED);

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
    const int64_t foreign_task = AddTaskForUser(kOtherUserId, "bob-task"sv, TaskStatus::ACTIVE);

    EXPECT_THROW(db.EditTask(kUserId, foreign_task, "hack"s), std::runtime_error);
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
    const int64_t foreign_task = AddTaskForUser(kOtherUserId, "foreign"sv, TaskStatus::ACTIVE);

    EXPECT_THROW(db.DeleteTask(kUserId, foreign_task), std::runtime_error);
}

TEST_F(TaskDBTest, DeleteAllUserTasks_Removes_Only_Specified_Users_Tasks) {
    AddDefaultUser();
    db.AddUser(kOtherUserId, "bob"s);
    AddTaskForDefaultUser("a"sv, TaskStatus::ACTIVE);
    AddTaskForDefaultUser("b"sv, TaskStatus::COMPLETED);
    static_cast<void>(AddTaskForUser(kOtherUserId, "bob-task"sv, TaskStatus::ACTIVE));

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

TEST_F(TaskDBTest, BeginTransaction_Throws_When_Already_Active) {
    db.BeginTransaction();

    EXPECT_THROW(db.BeginTransaction(), std::logic_error);

    db.RollbackTransaction();
}

TEST_F(TaskDBTest, CommitTransaction_Throws_Without_Active_Transaction) {
    EXPECT_THROW(db.CommitTransaction(), std::logic_error);
}

TEST_F(TaskDBTest, RollbackTransaction_Throws_Without_Active_Transaction) {
    EXPECT_THROW(db.RollbackTransaction(), std::logic_error);
}

TEST_F(TaskDBTest, CreateTransactionGuard_Throws_Inside_ManualTransaction) {
    db.BeginTransaction();

    EXPECT_THROW(static_cast<void>(db.CreateTransactionGuard()), std::runtime_error);

    db.RollbackTransaction();
}

TEST_F(TaskDBTest, RollbackTransaction_Reverts_Changes_After_Error_In_Transaction) {
    db.BeginTransaction();
    db.AddUser(kUserId, "alice"s);

    EXPECT_THROW(db.AddUser(kUserId, "alice-duplicate"s), std::runtime_error);

    db.RollbackTransaction();

    EXPECT_FALSE(db.IsUserExist(kUserId));
}

TEST_F(TaskDBTest, ConcurrentAddUser_Adds_All_Unique_Users) {
    constexpr int kThreadCount = 8;
    constexpr int64_t kBaseUserId = 10'000;

    std::latch ready_latch(kThreadCount);
    std::latch start_latch(1);
    std::mutex errors_mutex;
    std::vector<std::string> errors;
    std::vector<std::jthread> threads;
    threads.reserve(kThreadCount);

    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([&, i]() {
            ready_latch.count_down();
            start_latch.wait();
            try {
                db.AddUser(kBaseUserId + i, std::format("user-{}", i));
            } catch (const std::exception& e) {
                const std::lock_guard<std::mutex> lock(errors_mutex);
                errors.emplace_back(e.what());
            }
        });
    }

    ready_latch.wait();
    start_latch.count_down();

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(errors.empty());
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_TRUE(db.IsUserExist(kBaseUserId + i));
    }
}

TEST_F(TaskDBTest, ConcurrentUpdateTaskStatus_Does_Not_Throw_And_Keeps_Task_Visible) {
    AddDefaultUser();
    const int64_t task_id = AddTaskForDefaultUser("toggle-status"sv, TaskStatus::ACTIVE);
    constexpr int kThreadCount = 10;

    std::latch ready_latch(kThreadCount);
    std::latch start_latch(1);
    std::atomic<int> exception_count = 0;
    std::vector<std::jthread> threads;
    threads.reserve(kThreadCount);

    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([&, i]() {
            ready_latch.count_down();
            start_latch.wait();
            try {
                const TaskStatus new_status =
                    (i % 2 == 0) ? TaskStatus::ACTIVE : TaskStatus::COMPLETED;
                db.UpdateTaskStatus(kUserId, task_id, new_status);
            } catch (...) {
                ++exception_count;
            }
        });
    }

    ready_latch.wait();
    start_latch.count_down();

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(exception_count.load(), 0);
    const auto tasks = db.GetAllTasks(kUserId);
    ASSERT_TRUE(tasks.has_value());
    ASSERT_EQ(tasks->size(), 1U);
    EXPECT_TRUE((*tasks)[0].status == TaskStatus::ACTIVE ||
                (*tasks)[0].status == TaskStatus::COMPLETED);
}

TEST_F(TaskDBTest, ConcurrentReadAndWriteStatistics_Remains_Consistent) {
    AddDefaultUser();
    constexpr int kTaskCount = 20;
    constexpr int kReaderCount = 4;
    for (int i = 0; i < kTaskCount; ++i) {
        const TaskStatus status = (i % 2 == 0) ? TaskStatus::ACTIVE : TaskStatus::COMPLETED;
        static_cast<void>(AddTaskForDefaultUser(std::format("task-{}", i), status));
    }

    std::atomic<bool> writer_done = false;
    std::atomic<bool> inconsistent_stats = false;
    std::atomic<int> exception_count = 0;

    std::jthread writer([&]() {
        try {
            for (int i = 0; i < kTaskCount * 5; ++i) {
                const int64_t task_id = static_cast<int64_t>(i % kTaskCount) + 1;
                const TaskStatus status = (i % 2 == 0) ? TaskStatus::ACTIVE : TaskStatus::COMPLETED;
                db.UpdateTaskStatus(kUserId, task_id, status);
            }
        } catch (...) {
            ++exception_count;
        }
        writer_done = true;
    });

    std::vector<std::jthread> readers;
    readers.reserve(kReaderCount);
    for (int i = 0; i < kReaderCount; ++i) {
        readers.emplace_back([&]() {
            try {
                while (!writer_done.load()) {
                    const TaskStatistics stats = db.GetUserStatistics(kUserId);
                    if (stats.total != kTaskCount ||
                        stats.active + stats.completed != stats.total) {
                        inconsistent_stats = true;
                    }
                }
            } catch (...) {
                ++exception_count;
            }
        });
    }

    writer.join();
    for (auto& reader : readers) {
        reader.join();
    }

    EXPECT_EQ(exception_count.load(), 0);
    EXPECT_FALSE(inconsistent_stats.load());
}

}  // namespace
