cmake_minimum_required(VERSION 3.20)
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

project(TAGEngine)

add_library(TAGEngine STATIC tag_engine\src)

target_include_directories(TAGEngine PUBLIC tag_engine\include\vendor tag_engine\include\tag)

target_link_libraries(TAGEngine PUBLIC glfw)

target_compile_features(TAGEngine PUBLIC cxx_std_20)