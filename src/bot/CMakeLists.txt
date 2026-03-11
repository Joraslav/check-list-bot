file(GLOB BOT_INCLUDE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB BOT_SOURCE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/keyboards)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/handlers)

add_library(bot STATIC
    ${BOT_INCLUDE}
    ${BOT_SOURCE}
)

target_include_directories(bot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bot
    PUBLIC
        database
        keyboard
        handler
        tgbot::tgbot
    PRIVATE
        magic_enum::magic_enum
        utils
        log
)

if(TARGET bot)
    message(STATUS "Lib Bot ready to compile")
else()
    message(FATAL_ERROR "Lib Bot can not to be ready to compile! Errors")
endif()
