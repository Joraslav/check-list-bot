file(GLOB DATABASE_INCLUDE *.hpp *.h)
file(GLOB DATABASE_SOURCE *.cpp)

add_library(database STATIC
    ${DATABASE_INCLUDE}
    ${DATABASE_SOURCE}
)

target_include_directories(database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(database PUBLIC SQLiteCpp)
target_include_directories(database PUBLIC magic_enum::magic_enum)
target_include_directories(database PUBLIC ${CMAKE_SOURCE_DIR}/src/utils)
target_include_directories(database PUBLIC ${CMAKE_SOURCE_DIR}/src/log)

target_link_libraries(database SQLiteCpp)
target_link_libraries(database magic_enum::magic_enum)
target_link_libraries(database utils)
target_link_libraries(database log)

if(TARGET database)
    message(STATUS "Lib DataBase ready to compile")
else()
    message(FATAL_ERROR "Lib DataBase can not to be ready to compile! Errors")
endif()
