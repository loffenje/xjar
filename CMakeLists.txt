cmake_minimum_required(VERSION 3.6)

project(xjar)

set(CMAKE_CXX_STANDARD 20)

if(NOT DEFINED RENDERER_BACKEND)
    set(RENDERER_BACKEND "OpenGL") # Default value
endif()

if(NOT RENDERER_BACKEND STREQUAL "OpenGL" AND NOT RENDERER_BACKEND STREQUAL "Vulkan")
    message(FATAL_ERROR "Supported renderer: OpenGL, Vulkan")
endif()

message(STATUS "Using renderer: ${RENDERER_BACKEND}")


if(RENDERER_BACKEND STREQUAL "OpenGL")
    find_package(OpenGL REQUIRED)
    set(RENDERER_SRC
        src/renderer/gl/opengl_backend.cpp)
    add_subdirectory(3rd/glad)
    include_directories(3rd/glad/include)
elseif(RENDERER_BACKEND STREQUAL "Vulkan")
    find_package(Vulkan REQUIRED)
    set(RENDERER_SRC
        src/renderer/vk/vulkan_pipeline.cpp
        src/renderer/vk/vulkan_backend.cpp)
endif()

set(SRCS 
    src/glfw_platform.cpp
    src/window.cpp
    src/texture_manager.cpp
    src/renderer/renderer_system.cpp
    ${RENDERER_SRC})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(3rd/glfw)
add_subdirectory(3rd/glm)

set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(3rd/assimp 3rd/assimp/build)

if (NOT TARGET glm::glm)
    find_package(glm CONFIG REQUIRED)
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(3rd)

add_executable(${PROJECT_NAME} ${SRCS})

target_compile_definitions(${PROJECT_NAME} PRIVATE RENDERER_BACKEND=${RENDERER_BACKEND})

if (RENDERER_BACKEND STREQUAL "OpenGL")
target_link_libraries(${PROJECT_NAME}
    PRIVATE glad          # Private means not exposed to dependents of xjar
    PRIVATE glfw
    PRIVATE glm::glm-header-only
    PRIVATE assimp
    PRIVATE zlibstatic
)
elseif(RENDERER_BACKEND STREQUAL "Vulkan")

target_link_libraries(${PROJECT_NAME}
    PRIVATE glfw
    PRIVATE Vulkan::Vulkan
    PRIVATE glm::glm-header-only
    PRIVATE assimp
    PRIVATE zlibstatic
)
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)
