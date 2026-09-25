cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
project(TAGEngine)
include(FetchContent)

# GLFW options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Build with executable, not as a library
# Create a "test" folder for executable location
option(TAG_BUILD_EXECUTABLE "Build with executable, not as a library" OFF)

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
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/test)
    file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/test/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
    )
    add_executable(testing ${GAME_SOURCES})
    target_link_libraries(testing PRIVATE TAGEngine)
endif()
