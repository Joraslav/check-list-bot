cmake_minimum_required(VERSION 3.31.0)
project(check-list-bot
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Check-list-bot is a Telegram bot for management of tasks.")

enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MODE_DEV "Enable test/dev targets" ON)
option(ENABLE_CCACHE "Use ccache to speed up C/C++ rebuilds" ON)
if(ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        message(STATUS "ccache enabled: ${CCACHE_PROGRAM}")
    endif()
endif()

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

if(MODE_DEV)
    FIND_LIB(GTest)
    add_subdirectory(tests)
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE bot)

