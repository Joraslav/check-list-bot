#include "Bot.hpp"

#include "tgbot/net/TgLongPoll.h"

#include "InlineKeyboardHandler.hpp"
#include "Log.hpp"
#include "ReplyKeyboardHandler.hpp"
#include "SlashCommandKeyboardHandler.hpp"
#include "TaskDB.hpp"

#include <csignal>
#include <cstdint>
#include <exception>

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

}  // namespace

namespace bot {

Bot::Bot(std::string_view token, const std::filesystem::path& db_path,
         int32_t long_poll_timeout_sec)
    : long_poll_timeout_sec_(long_poll_timeout_sec) {
    tg_bot_ = std::make_unique<TgBot::Bot>(token.data());
    db_ = std::make_unique<database::TaskDB>(db_path.string());

    LOG(BotLog, INFO, "Bot initialized with long poll timeout {} sec", long_poll_timeout_sec_);
}

Bot::~Bot() { LOG(BotLog, INFO, "Bot shutting down"); }

void Bot::SetupHandlers() {
    SlashCommandKeyboardHandler::Register(*tg_bot_);
    ReplyKeyboardHandler::Register(*tg_bot_);
    InlineKeyboardHandler::Register(*tg_bot_);
}

void Bot::Run() {
    SetupHandlers();
    g_stop_requested = 0;
    const SignalHandlerGuard signal_handler_guard;

    TgBot::TgLongPoll long_poll(*tg_bot_, kLongPollLimit, long_poll_timeout_sec_);
    LOG(BotLog, INFO, "Bot started long polling");
    RunLongPolling(long_poll);

    LOG(BotLog, INFO, "Shutdown signal received, long polling stopped");
}

}  // namespace bot
