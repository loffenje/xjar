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
add_subdirectory(3rd/glm)

set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(3rd/assimp 3rd/assimp/build)

if (NOT TARGET glm::glm)
    find_package(glm CONFIG REQUIRED)
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(3rd)
include_directories(3rd/glad/include)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE glad          # Private means not exposed to dependents of xjar
    PRIVATE glfw
    PRIVATE glm::glm-header-only
    PRIVATE assimp
    PRIVATE zlibstatic
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)
