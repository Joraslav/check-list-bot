cmake_minimum_required(VERSION 3.31.0)
project(check-list-keyboard
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Check-list-keyboard is a Telegram keyboard for managment of tasks.")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
add_subdirectory(src/database)
add_subdirectory(src/log)
add_subdirectory(src/bot)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE bot)

