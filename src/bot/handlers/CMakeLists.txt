file(GLOB HANDLER_INCLUDE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB HANDLER_SOURCE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(handler STATIC
    ${HANDLER_INCLUDE}
    ${HANDLER_SOURCE}
)

target_include_directories(handler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(handler PUBLIC ${CMAKE_SOURCE_DIR}/src/bot)
target_include_directories(handler PUBLIC ${CMAKE_SOURCE_DIR}/src/bot/keyboards)
target_link_libraries(handler
    PUBLIC
        database
        keyboard
    PRIVATE
        tgbot::tgbot
        log
        magic_enum::magic_enum
)

if(TARGET handler)
    message(STATUS "Lib Handlers ready to compile")
else()
    message(FATAL_ERROR "Lib Handlers can not to be ready to compile! Errors")
endif()
