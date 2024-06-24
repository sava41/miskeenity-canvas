cmake_minimum_required(VERSION 3.28)
project(miskeenity-canvas LANGUAGES CXX C)

if (NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
endif()

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/third_party)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
include(ExternalProject)
include(${PROJECT_SOURCE_DIR}/cmake/EmbedFile.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

# Linux -pthread shenanigans
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

# Add third party libs
if (NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    add_subdirectory(third_party/webgpu)
endif()
add_subdirectory(third_party/iconfontheaders)
add_subdirectory(third_party/sdl3)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/stb)

# Add resources
embed_files(embedded_files 
    ./resources/fonts/Lucide.ttf 
    ./resources/fonts/Roboto.ttf 
    ./resources/shaders/layers.wgsl 
    ./resources/shaders/selection.wgsl)

# Setup executable
add_executable(miskeenity-canvas 
    source/main.cpp 
    source/graphics.cpp 
    source/layers.cpp
    source/ui.cpp
    source/image.cpp
    source/texture_manager.cpp
    source/webgpu_surface.c)

add_dependencies(miskeenity-canvas SDL3::SDL3 imgui glm::glm stb icon-font-headers embedded_files)
target_link_libraries(miskeenity-canvas PRIVATE SDL3::SDL3 imgui glm::glm stb icon-font-headers embedded_files)
target_compile_definitions(miskeenity-canvas PRIVATE SDL_MAIN_USE_CALLBACKS)

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # use minimal shell.html
    target_link_options(miskeenity-canvas PRIVATE --shell-file ${PROJECT_SOURCE_DIR}/resources/shell/shell.html)
    # wasm options
    target_link_options(miskeenity-canvas PRIVATE -sUSE_WEBGPU=1 -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY)
    # WASM+JS size optimizations
    #target_link_options(miskeenity-canvas PRIVATE -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sMALLOC=emmalloc --closure=1)
else()
    add_dependencies(miskeenity-canvas webgpu_cpp webgpu_dawn)
    target_link_libraries(miskeenity-canvas PRIVATE webgpu_cpp webgpu_dawn)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set_source_files_properties(source/webgpu_surface.c PROPERTIES COMPILE_FLAGS "-x objective-c")
    target_link_libraries(miskeenity-canvas
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework Metal"
    )
endif()

# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_link_options(miskeenity-canvas PRIVATE LINKER:-dead_strip)
endif()
