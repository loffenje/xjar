cmake_minimum_required(VERSION 3.6)

project(xjar)

set(CMAKE_CXX_STANDARD 20)

set(SRCS src/glfw_platform.cpp)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)

add_subdirectory(3rd/glfw)
add_subdirectory(3rd/glad)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(3rd)
include_directories(3rd/glad/include)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} glad)
target_link_libraries(${PROJECT_NAME} glfw)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)
