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

# Path to test folder
set(TAG_TESTING_PATH ${CMAKE_SOURCE_DIR}/test/)

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
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TAG_TESTING_PATH})
    file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
        ${TAG_TESTING_PATH}*.cpp
        ${TAG_TESTING_PATH}*.c
    )
    add_executable(testing ${GAME_SOURCES})
    target_link_libraries(testing PRIVATE TAGEngine)
endif()
