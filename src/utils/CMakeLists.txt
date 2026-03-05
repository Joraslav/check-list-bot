file(GLOB UTILS_INCLUDE *.hpp *.h)
file(GLOB UTILS_SOURCE *.cpp)

add_library(utils STATIC
    ${UTILS_INCLUDE}
    ${UTILS_SOURCE}
)

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(TARGET utils)
    message(STATUS "Lib Utils ready to compile")
else()
    message(FATAL_ERROR "Lib Utils can not to be ready to compile! Errors")
endif()