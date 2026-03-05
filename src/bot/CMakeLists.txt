file(GLOB BOT_INCLUDE *.hpp *.h)
file(GLOB BOT_SOURCE *.cpp)

add_library(bot STATIC
    ${BOT_INCLUDE}
    ${BOT_SOURCE}
)

target_include_directories(bot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bot PUBLIC tgBot::tgBot)
target_include_directories(bot PUBLIC magic_enum::magic_enum)
target_include_directories(bot PUBLIC ${CMAKE_SOURCE_DIR}/src/utils)
target_include_directories(bot PUBLIC database)

target_link_libraries(bot tgbot::tgbot)
target_link_libraries(bot utils)
target_link_libraries(bot database)
target_link_libraries(bot magic_enum::magic_enum)

if(TARGET bot)
    message(STATUS "Lib Bot ready to compile")
else()
    message(FATAL_ERROR "Lib Bot can not to be ready to compile! Errors")
endif()
