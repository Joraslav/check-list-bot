file(GLOB LOG_INCLUDE *.hpp *.h)
file(GLOB LOG_SOURCE *.cpp)

add_library(log STATIC
    ${LOG_INCLUDE}
    ${LOG_SOURCE}
)

target_include_directories(log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(log PUBLIC spdlog::spdlog)
target_include_directories(log PUBLIC ${CMAKE_SOURCE_DIR}/src/utils)

target_link_libraries(log spdlog::spdlog)
target_link_libraries(log utils)

if(TARGET log)
    message(STATUS "Lib Log ready to compile")
else()
    message(FATAL_ERROR "Lib Log can not to be ready to compile! Errors")
endif()
