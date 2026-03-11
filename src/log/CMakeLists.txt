file(GLOB LOG_INCLUDE CONFIGURE_DEPENDS *.hpp *.h)
file(GLOB LOG_SOURCE CONFIGURE_DEPENDS *.cpp)

add_library(log STATIC
    ${LOG_INCLUDE}
    ${LOG_SOURCE}
)

target_include_directories(log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(log
    PUBLIC
        spdlog::spdlog
)

if(TARGET log)
    message(STATUS "Lib Log ready to compile")
else()
    message(FATAL_ERROR "Lib Log can not to be ready to compile! Errors")
endif()
