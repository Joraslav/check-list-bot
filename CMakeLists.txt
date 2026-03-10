cmake_minimum_required(VERSION 3.31.0)
project(check-list-bot
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Check-list-bot is a Telegram bot for managment of tasks.")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall
    -Wextra
    -pedantic
    -Wno-unused-parameter
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/functions.cmake)

FIND_LIB(spdlog)
FIND_LIB(SQLiteCpp)
FIND_LIB(tgbot)
FIND_LIB(magic_enum)

add_subdirectory(src/utils)
add_subdirectory(src/bot)
add_subdirectory(src/database)
add_subdirectory(src/log)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/log)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/bot)
target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} bot)

