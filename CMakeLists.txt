cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(TAGEngine)
include(FetchContent)

# GLFW options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Build with executable, not as a library
option(TAG_BUILD_EXECUTABLE "Build with executable, not as a library" ON)

# Path to test source code and executable output
set(TAG_TEST_SOURCE ${CMAKE_SOURCE_DIR}/test/src/)
set(TAG_TEST_EXE ${CMAKE_SOURCE_DIR}/test/)

# Fetch GLFW 
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# Build engine
file(GLOB_RECURSE TAG_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tag_engine/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tag_engine/src/*.c
)
add_library(TAGEngine STATIC ${TAG_SOURCES})
target_include_directories(TAGEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tag_engine/include/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/tag_engine/include/tag
)
target_link_libraries(TAGEngine PUBLIC glfw)

# Build executable if enabled
if(TAG_BUILD_EXECUTABLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TAG_TEST_EXE})
    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        ${TAG_TEST_SOURCE}*.cpp
        ${TAG_TEST_SOURCE}*.c
    )
    add_executable(testing ${TEST_SOURCES})
    target_link_libraries(testing PRIVATE TAGEngine)
endif()
