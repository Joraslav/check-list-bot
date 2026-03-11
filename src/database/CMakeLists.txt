file(GLOB DATABASE_INCLUDE CONFIGURE_DEPENDS *.hpp *.h)
file(GLOB DATABASE_SOURCE CONFIGURE_DEPENDS *.cpp)

add_library(database STATIC
    ${DATABASE_INCLUDE}
    ${DATABASE_SOURCE}
)

target_include_directories(database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(database
    PUBLIC
        SQLiteCpp
    PRIVATE
        magic_enum::magic_enum
        utils
        log
)

if(TARGET database)
    message(STATUS "Lib DataBase ready to compile")
else()
    message(FATAL_ERROR "Lib DataBase can not to be ready to compile! Errors")
endif()
