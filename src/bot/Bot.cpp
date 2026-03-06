#include "Bot.hpp"

#include "tgbot/tgbot.h"

#include "Log.hpp"
#include "magic_enum/magic_enum.hpp"
#include "TaskDB.hpp"

#include "handlers/ConsoleHandler.hpp"
#include "handlers/ReplyKeyboardHandler.hpp"
#include "handlers/InlineKeyboardHandler.hpp"

#include <format>
#include <memory>
#include <string>

DEFINE_LOG_CATEGORY_STATIC(BotLog);

namespace bot {

Bot::Bot(std::string_view token, std::string_view db_path) {
    tg_bot_ = std::make_unique<TgBot::Bot>(token.data());
    db_ = std::make_unique<database::TaskDB>(db_path);

    LOG(BotLog, INFO, "Bot initialized!");
}

Bot::~Bot() { LOG(BotLog, INFO, "Bot shutting down"); }

void Bot::SetupHandlers() {
    ConsoleHandler::Register(*tg_bot_);
    ReplyKeyboardHandler::Register(*tg_bot_);
    InlineKeyboardHandler::Register(*tg_bot_);
}

void Bot::Run() {
    SetupHandlers();

    TgBot::TgLongPoll long_poll(*tg_bot_);
    LOG(BotLog, INFO, "Bot started long polling");
    while (true) {
        try {
            long_poll.start();
        } catch (const TgException& e) {
            LOG(BotLog, ERROR, "Tg error: {}", e.what());
        } catch (const std::exception& e) {
            LOG(BotLog, ERROR, "Long poll error: {}", e.what());
        }
    }
}

}  // namespace bot