cmake_minimum_required(VERSION 3.15)
project(villainy)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Vulkan SDK
find_package(Vulkan REQUIRED)

# GLFW (binary)
set(INCLUDE_ROOT ${CMAKE_SOURCE_DIR}/external)

include_directories(${INCLUDE_ROOT}/include)
link_directories(${INCLUDE_ROOT}/lib)

set(VILLAINY_SOURCES
    src/villainy/villainy.cpp
    src/villainy/context.cpp
    src/villainy/window.cpp
    src/villainy/logger.cpp
    src/villainy/utils.cpp
    src/villainy/buffer.cpp
    src/villainy/command.cpp
    src/villainy/descriptorManager.cpp
    src/villainy/render.cpp
    src/villainy/renderpass.cpp
    src/villainy/shader.cpp
    src/villainy/swapchain.cpp
    src/villainy/texture.cpp
)

add_library(VillainyLib ${VILLAINY_SOURCES})

# executable
add_executable(villainy src/main.cpp)

# compile flags per config
target_compile_definitions(villainy PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

target_compile_options(villainy PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
    $<$<CONFIG:Release>:-O3>
)

# link libraries
target_link_libraries(villainy
    Vulkan::Vulkan
    #MoltenVK
    glfw.3           # <-- IMPORTANT: matches libglfw.3.dylib
    VillainyLib
)