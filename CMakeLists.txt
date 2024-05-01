cmake_minimum_required(VERSION 3.28)
project(miskeenity-canvas LANGUAGES C CXX)

find_package(Python REQUIRED Interpreter Development)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/third_party)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
include(ExternalProject)
include(${PROJECT_SOURCE_DIR}/cmake/EmbedFile.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
# Linux -pthread shenanigans
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

# Add third party libs
add_subdirectory(third_party/iconfontheaders)
add_subdirectory(third_party/sdl3)
add_subdirectory(third_party/webgpu)
add_subdirectory(third_party/imgui)

# Add resources
embed_files(embedded_files ./resources/fonts/Lucide.ttf ./resources/fonts/Roboto.ttf)

# Setup executable
add_executable(miskeenity-canvas source/main.cpp source/webgpu_surface.c)

add_dependencies(miskeenity-canvas SDL3::SDL3 imgui webgpu icon-font-headers embedded_files)
target_link_libraries(miskeenity-canvas PRIVATE SDL3::SDL3 imgui webgpu icon-font-headers embedded_files)
target_compile_definitions(miskeenity-canvas PRIVATE SDL_MAIN_USE_CALLBACKS)

# Emscripten-specific linker options
if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # use our own minimal shell.html
    target_link_options(miskeenity-canvas PRIVATE --shell-file ${PROJECT_SOURCE_DIR}/resources/shell/shell.html)
    # wasm options
    target_link_options(miskeenity-canvas PRIVATE -sUSE_WEBGPU=1 -sALLOW_MEMORY_GROWTH=1)
    # WASM+JS size optimizations
    #target_link_options(miskeenity-canvas PRIVATE -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sMALLOC=emmalloc --closure=1)
endif()

# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_link_options(miskeenity-canvas PRIVATE LINKER:-dead_strip)
endif()
