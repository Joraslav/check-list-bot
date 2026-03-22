cmake_minimum_required(VERSION 3.31.0)

# Find GTest
find_package(GTest REQUIRED)
include(GoogleTest)

# Enable testing
enable_testing()

# Get all test files
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

# Create a test executable for each test file
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    
    add_executable(${test_name} ${test_source})
    
    # Link test executable with GTest and project libraries
    target_link_libraries(${test_name}
        PRIVATE
            GTest::gtest
            GTest::gtest_main
            database
            bot
            keyboard
            handler
            utils
            log
    )
    
    # Add include directories
    target_include_directories(${test_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/database
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/bot
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/bot/keyboards
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/bot/handlers
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/log
    )
    
    # Register test
    gtest_discover_tests(${test_name}
        TEST_PREFIX "${test_name}."
    )
endforeach()

message(STATUS "Tests configured: ${TEST_SOURCES}")
