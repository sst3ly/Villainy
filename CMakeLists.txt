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

add_library(VillainyLib_static ${VILLAINY_SOURCES})
add_library(VillainyLib_shared ${VILLAINY_SOURCES})
set_target_properties(VillainyLib_static PROPERTIES OUTPUT_NAME "VillainyLib")
set_target_properties(VillainyLib_shared PROPERTIES OUTPUT_NAME "VillainyLib")
set_target_properties(VillainyLib_shared PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin # .dll
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib # .lib/.a
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib # .so/.dylib
)

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
    glfw.3           # <-- IMPORTANT: matches libglfw.3.dylib
    VillainyLib_static
)