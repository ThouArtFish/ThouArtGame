cmake_minimum_required(VERSION 3.20)
project(TAGEngine)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

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
target_compile_features(TAGEngine PUBLIC cxx_std_20)