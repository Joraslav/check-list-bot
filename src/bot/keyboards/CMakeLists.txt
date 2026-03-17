file(GLOB KEYBOARD_INCLUDE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB KEYBOARD_SOURCE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(keyboard STATIC
    ${KEYBOARD_INCLUDE}
    ${KEYBOARD_SOURCE}
)

target_include_directories(keyboard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(keyboard
    PUBLIC
        database
        tgbot::tgbot
    PRIVATE
        magic_enum::magic_enum
)

if(TARGET keyboard)
    message(STATUS "Lib Keyboards ready to compile")
else()
    message(FATAL_ERROR "Lib Keyboards can not to be ready to compile! Errors")
endif()
