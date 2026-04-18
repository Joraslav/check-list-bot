#include "Bot.hpp"

#include "tgbot/net/CurlHttpClient.h"
#include "tgbot/net/TgLongPoll.h"

#include "InlineKeyboardHandler.hpp"
#include "Log.hpp"
#include "ReplyKeyboardHandler.hpp"
#include "SlashCommandKeyboardHandler.hpp"
#include "TaskDB.hpp"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <thread>

DEFINE_LOG_CATEGORY_STATIC(BotLog);

namespace {

constexpr int32_t kLongPollLimit = 100;

volatile std::sig_atomic_t g_stop_requested = 0;

[[nodiscard]] bool IsStopRequested() noexcept { return g_stop_requested != 0; }

void HandleShutdownSignal(int signal_number) {
    if (signal_number == SIGINT || signal_number == SIGTERM) {
        g_stop_requested = 1;
    }
}

class SignalHandlerGuard final {
 public:
    SignalHandlerGuard()
        : old_sigint_handler_(std::signal(SIGINT, HandleShutdownSignal)),
          old_sigterm_handler_(std::signal(SIGTERM, HandleShutdownSignal)) {}

    ~SignalHandlerGuard() noexcept {
        std::signal(SIGINT, old_sigint_handler_);
        std::signal(SIGTERM, old_sigterm_handler_);
    }

    SignalHandlerGuard(const SignalHandlerGuard&) = delete;
    SignalHandlerGuard& operator=(const SignalHandlerGuard&) = delete;

 private:
    using SignalHandler = void (*)(int);

    SignalHandler old_sigint_handler_ = SIG_DFL;
    SignalHandler old_sigterm_handler_ = SIG_DFL;
};

bool RunLongPollIteration(TgBot::TgLongPoll& long_poll) {
    try {
        long_poll.start();
        return true;
    } catch (const TgBot::TgException& e) {
        if (IsStopRequested()) {
            return false;
        }
        LOG(BotLog, ERROR, "Tg error: {}", e.what());
        return true;
    } catch (const std::exception& e) {
        if (IsStopRequested()) {
            return false;
        }
        LOG(BotLog, ERROR, "Long poll error: {}", e.what());
        return true;
    }
}

void RunLongPolling(TgBot::TgLongPoll& long_poll) {
    while (!IsStopRequested()) {
        if (!RunLongPollIteration(long_poll)) {
            break;
        }
    }
}

bool TryDeleteWebhookOnce(TgBot::Bot& bot, int32_t attempt, int32_t attempts) {
    try {
        bot.getApi().deleteWebhook(/*dropPendingUpdates=*/true);
        LOG(BotLog, INFO, "Webhook deleted, pending updates dropped");
        return true;
    } catch (const TgBot::TgException& e) {
        LOG(BotLog, WARNING, "deleteWebhook attempt {}/{} failed: {}", attempt, attempts, e.what());
    } catch (const std::exception& e) {
        LOG(BotLog, WARNING, "deleteWebhook attempt {}/{} failed: {}", attempt, attempts, e.what());
    }

    return false;
}

void CleanupWebhookWithRetry(TgBot::Bot& bot, int32_t max_attempts) {
    const int32_t attempts = (max_attempts > 0) ? max_attempts : 1;
    for (int32_t attempt = 1; attempt <= attempts; ++attempt) {
        if (TryDeleteWebhookOnce(bot, attempt, attempts)) {
            return;
        }

        if (attempt < attempts) {
            const auto delay = std::chrono::seconds(1 << (attempt - 1));
            std::this_thread::sleep_for(delay);
        }
    }

    LOG(BotLog, WARNING, "deleteWebhook failed after {} attempts, continue without cleanup",
        attempts);
}

}  // namespace

namespace bot {

Bot::Bot(std::string_view token, const std::filesystem::path& db_path,
         int32_t long_poll_timeout_sec, bool clear_webhook_on_start,
         int32_t webhook_cleanup_max_attempts)
    : long_poll_timeout_sec_(long_poll_timeout_sec),
      clear_webhook_on_start_(clear_webhook_on_start),
      webhook_cleanup_max_attempts_(webhook_cleanup_max_attempts) {
    http_client_ = std::make_unique<TgBot::CurlHttpClient>();
    tg_bot_ = std::make_unique<TgBot::Bot>(std::string(token), *http_client_);
    db_ = std::make_unique<database::TaskDB>(db_path.string());

    LOG(BotLog, INFO, "Bot initialized with long poll timeout {} sec using libcurl HTTP client",
        long_poll_timeout_sec_);
}

Bot::~Bot() { LOG(BotLog, INFO, "Bot shutting down"); }

void Bot::SetupHandlers() {
    SlashCommandKeyboardHandler::Register(*tg_bot_, *db_);
    ReplyKeyboardHandler::Register(*tg_bot_);
    InlineKeyboardHandler::Register(*tg_bot_);
}

void Bot::Run() {
    SetupHandlers();
    g_stop_requested = 0;
    const SignalHandlerGuard signal_handler_guard;

    if (clear_webhook_on_start_) {
        LOG(BotLog, INFO, "Starting webhook cleanup with up to {} attempts",
            webhook_cleanup_max_attempts_);
        CleanupWebhookWithRetry(*tg_bot_, webhook_cleanup_max_attempts_);
    } else {
        LOG(BotLog, INFO, "Webhook cleanup on startup is disabled");
    }

    TgBot::TgLongPoll long_poll(*tg_bot_, kLongPollLimit, long_poll_timeout_sec_);
    LOG(BotLog, INFO, "Bot started long polling");
    RunLongPolling(long_poll);

    LOG(BotLog, INFO, "Shutdown signal received, long polling stopped");
}

}  // namespace bot
