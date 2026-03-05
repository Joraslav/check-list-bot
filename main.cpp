#include "Log.hpp"
#include "Bot.hpp"

#include <iostream>

DEFINE_LOG_CATEGORY_STATIC(FirsLog);

int main(int, char**) {
    LOG(FirsLog, INFO, "Hello, {}", 2);
    LOG(FirsLog, DEBUG, "This degus string");
    LOG(FirsLog, ERROR, "This is an error message");
    LOG(FirsLog, WARNING, "This is a warning message");
    
    const std::string token = std::getenv("TOKEN");

    return 0;
}
