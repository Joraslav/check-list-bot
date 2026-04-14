#include "gtest/gtest.h"

#include "Log.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace logging;
using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

namespace fs = std::filesystem;

namespace {

/**
 * @brief RAII-хелпер для управления временной директорией.
 * Директория удаляется в деструкторе с помощью fs::remove_all.
 */
class TemporaryDirectory {
 public:
    explicit TemporaryDirectory(std::string_view suffix) : path_(MakeUniquePath(suffix)) {}

    ~TemporaryDirectory() {
        std::error_code ec;
        fs::remove_all(path_, ec);
        // Игнорируем ошибки удаления (файловая система может быть недоступна)
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
    TemporaryDirectory(TemporaryDirectory&&) = default;
    TemporaryDirectory& operator=(TemporaryDirectory&&) = default;

    [[nodiscard]] const fs::path& Get() const { return path_; }

    // Конверсия к fs::path для удобства
    operator const fs::path&() const { return path_; }

 private:
    static fs::path MakeUniquePath(std::string_view suffix) {
        const auto unique_part =
            std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        const auto tick_part =
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        return fs::temp_directory_path() /
               std::format("check-list-bot-log-test-{}-{}-{}", suffix, unique_part, tick_part);
    }

    fs::path path_;
};

class LogTest : public ::testing::Test {
 protected:
    static constexpr std::string_view kCategoryName = "LogTestCategory"sv;

    static std::vector<fs::path> CollectLogFiles(const fs::path& dir) {
        std::vector<fs::path> files;
        if (!fs::exists(dir)) {
            return files;
        }

        for (const auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto& path = entry.path();
            if (path.extension() == ".log" && path.filename().string().starts_with("BotLog_"sv)) {
                files.push_back(path);
            }
        }

        return files;
    }

    static std::string ReadFile(const fs::path& path) {
        std::ifstream in(path);
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    static bool AnyLogFileContains(const fs::path& dir, std::string_view needle) {
        const auto files = CollectLogFiles(dir);
        return std::ranges::any_of(
            files, [needle](const auto& file) { return ReadFile(file).contains(needle); });
    }
};

static_assert(kMinLevel == LogLevel::INFO);
static_assert(kMaxLevel == LogLevel::FATAL);
static_assert(ValidLogCategory<LogCategory>);
static_assert(LoggableMessage<std::string>);
static_assert(LoggableMessage<std::string_view>);
static_assert(!LoggableMessage<int>);
static_assert(ValidLogLevel<LogLevel::INFO>);
static_assert(ValidLogLevel<LogLevel::DEBUG>);
static_assert(ValidLogLevel<LogLevel::WARNING>);
static_assert(ValidLogLevel<LogLevel::ERROR>);
static_assert(ValidLogLevel<LogLevel::FATAL>);

TEST_F(LogTest, LogConfig_DefaultValues_AreExpected) {
    const LogConfig config{};

    EXPECT_EQ(config.file_log_directory, fs::path("out/logs"));
    EXPECT_TRUE(config.enable_console_logging);
    EXPECT_TRUE(config.enable_file_logging);
}

TEST_F(LogTest, LogCategory_GetName_ReturnsConfiguredName) {
    const LogCategory category("unit-test"s);

    EXPECT_EQ(category.GetName(), "unit-test"s);
}

TEST_F(LogTest, GetInstance_ReturnsSameReferenceForAllCalls) {
    Log& first = Log::GetInstance();
    Log& second = Log::GetInstance();

    EXPECT_EQ(std::addressof(first), std::addressof(second));
}

TEST_F(LogTest, Configure_FileLoggingDisabled_DoesNotCreateLogDirectory) {
    TemporaryDirectory log_dir("disabled"sv);

    Log::GetInstance().Configure(LogConfig{.file_log_directory = log_dir.Get(),
                                           .enable_console_logging = false,
                                           .enable_file_logging = false});

    const LogCategory category{std::string(kCategoryName)};
    EXPECT_NO_THROW(Log::GetInstance().Loging(category, LogLevel::INFO, "message"sv));
    EXPECT_FALSE(fs::exists(log_dir.Get()));
}

TEST_F(LogTest, Configure_FileLoggingEnabled_CreatesFileAndWritesCategoryAndMessage) {
    TemporaryDirectory log_dir("enabled"sv);
    TemporaryDirectory finalize_dir("enabled-finalize"sv);
    const std::string payload = "payload-for-file-check"s;

    Log::GetInstance().Configure(LogConfig{.file_log_directory = log_dir.Get(),
                                           .enable_console_logging = false,
                                           .enable_file_logging = true});

    const LogCategory category{std::string(kCategoryName)};
    Log::GetInstance().Loging(category, LogLevel::WARNING, payload);

    // Reconfigure to force destruction of previous sinks and flush file buffers.
    Log::GetInstance().Configure(LogConfig{.file_log_directory = finalize_dir.Get(),
                                           .enable_console_logging = false,
                                           .enable_file_logging = false});

    const auto files = CollectLogFiles(log_dir.Get());
    ASSERT_FALSE(files.empty());
    EXPECT_TRUE(AnyLogFileContains(log_dir.Get(), "[LogTestCategory]"sv));
    EXPECT_TRUE(AnyLogFileContains(log_dir.Get(), payload));
}

TEST_F(LogTest, Loging_FatalLevel_DoesNotThrow) {
    TemporaryDirectory log_dir("fatal"sv);
    Log::GetInstance().Configure(LogConfig{.file_log_directory = log_dir.Get(),
                                           .enable_console_logging = false,
                                           .enable_file_logging = false});

    const LogCategory category("fatal-test"s);
    EXPECT_NO_THROW(Log::GetInstance().Loging(category, LogLevel::FATAL, "fatal-message"sv));
}

TEST_F(LogTest, Macros_DefineCategoryAndLog_WriteFormattedMessage) {
    TemporaryDirectory log_dir("macro"sv);
    TemporaryDirectory finalize_dir("macro-finalize"sv);
    Log::GetInstance().Configure(LogConfig{.file_log_directory = log_dir.Get(),
                                           .enable_console_logging = false,
                                           .enable_file_logging = true});

    DEFINE_LOG_CATEGORY_STATIC(LocalMacroCategory);
    LOG(LocalMacroCategory, INFO, "macro-value={} and text={}"sv, 42, "ok"s);

    // Reconfigure to force destruction of previous sinks and flush file buffers.
    Log::GetInstance().Configure(LogConfig{.file_log_directory = finalize_dir.Get(),
                                           .enable_console_logging = false,
                                           .enable_file_logging = false});

    const auto files = CollectLogFiles(log_dir.Get());
    ASSERT_FALSE(files.empty());
    EXPECT_TRUE(AnyLogFileContains(log_dir.Get(), "[LocalMacroCategory]"sv));
    EXPECT_TRUE(AnyLogFileContains(log_dir.Get(), "macro-value=42 and text=ok"sv));
}

TEST_F(LogTest, Configure_ConcurrentWithLoging_CompletesWithoutExceptions) {
    TemporaryDirectory init_dir("concurrent-init"sv);
    Log::GetInstance().Configure(LogConfig{.file_log_directory = init_dir.Get(),
                                           .enable_console_logging = false,
                                           .enable_file_logging = false});

    constexpr int kConfigureIterations = 150;
    constexpr int kLoggerThreads = 6;
    constexpr int kLogIterationsPerThread = 500;

    std::atomic<bool> has_error = false;

    std::vector<TemporaryDirectory> thread_dirs;
    thread_dirs.reserve(kConfigureIterations);

    std::thread configure_thread([&has_error, &thread_dirs]() {
        for (int i = 0; i < kConfigureIterations; ++i) {
            try {
                thread_dirs.emplace_back("concurrent"sv);
                Log::GetInstance().Configure(
                    LogConfig{.file_log_directory = thread_dirs.back().Get(),
                              .enable_console_logging = false,
                              .enable_file_logging = false});
            } catch (...) {
                has_error.store(true);
                return;
            }
        }
    });

    std::vector<std::thread> logger_threads;
    logger_threads.reserve(kLoggerThreads);
    for (int thread_index = 0; thread_index < kLoggerThreads; ++thread_index) {
        logger_threads.emplace_back([&has_error, thread_index]() {
            const LogCategory category{std::format("worker-{}", thread_index)};
            for (int i = 0; i < kLogIterationsPerThread; ++i) {
                try {
                    Log::GetInstance().Loging(category, LogLevel::DEBUG, "parallel-message"sv);
                } catch (...) {
                    has_error.store(true);
                    return;
                }
            }
        });
    }

    configure_thread.join();
    for (auto& t : logger_threads) {
        t.join();
    }

    EXPECT_FALSE(has_error.load());
}

}  // namespace
