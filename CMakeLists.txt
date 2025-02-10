cmake_minimum_required(VERSION 3.28)
project(miskeenity-canvas LANGUAGES CXX C)

if (APPLE)
    enable_language(OBJC)
endif()

if (NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
endif()

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/third_party)

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
    add_subdirectory(third_party/onnx)
endif()
add_subdirectory(third_party/iconfontheaders)
add_subdirectory(third_party/sdl3)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/stb)
add_subdirectory(third_party/emscripten-browser-file)
add_subdirectory(third_party/embed)

# Setup executable
add_executable(miskeenity-canvas 
    source/main.cpp 
    source/graphics.cpp
    source/events.cpp
    source/resource_manager.cpp
    source/layer_manager.cpp
    source/layer_history.cpp
    source/mesh_manager.cpp
    source/font_manager.cpp
    source/ui.cpp
    source/image.cpp
    source/sdl_utils.cpp
    source/texture_manager.cpp
    source/ml_inference.cpp
    source/webgpu_surface.c)

# Add resources
b_embed(miskeenity-canvas ./resources/fonts/Lucide_compact.ttf)
b_embed(miskeenity-canvas ./resources/fonts/Roboto.ttf)
b_embed(miskeenity-canvas ./resources/fonts/Arimo_compact.ttf)
b_embed(miskeenity-canvas ./resources/fonts/Anton_compact.ttf)
b_embed(miskeenity-canvas ./resources/fonts/EBGaramond_compact.ttf)
b_embed(miskeenity-canvas ./resources/shaders/layers.wgsl)
b_embed(miskeenity-canvas ./resources/shaders/postprocess.wgsl)
b_embed(miskeenity-canvas ./resources/shaders/selection.wgsl)
b_embed(miskeenity-canvas ./resources/shaders/mesh.wgsl)
b_embed(miskeenity-canvas ./resources/shaders/maskmultiply.wgsl)
b_embed(miskeenity-canvas ./resources/shaders/prealpha.wgsl)
b_embed(miskeenity-canvas ./resources/shaders/mipgen.wgsl)

add_dependencies(miskeenity-canvas SDL3::SDL3 imgui glm::glm stb icon-font-headers)
target_link_libraries(miskeenity-canvas PRIVATE SDL3::SDL3 imgui glm::glm stb icon-font-headers)
target_compile_definitions(miskeenity-canvas PRIVATE
    SDL_MAIN_USE_CALLBACKS
    $<$<NOT:$<CONFIG:Debug>>:B_PRODUCTION_MODE>
)

# Add git hash to build
set(GIT_HASH_VALUE "Uknown Version")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND git log -1 --pretty=format:%h
        OUTPUT_VARIABLE GIT_HASH_VALUE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
target_compile_definitions(miskeenity-canvas PRIVATE MC_GIT_HASH="${GIT_HASH_VALUE}")

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_dependencies(miskeenity-canvas emscripten-browser-file)
    target_link_libraries(miskeenity-canvas PRIVATE emscripten-browser-file)
    target_compile_definitions(miskeenity-canvas PRIVATE B_OS_WEB)
    # use minimal shell.html
    target_link_options(miskeenity-canvas PRIVATE --shell-file ${PROJECT_SOURCE_DIR}/resources/shell/shell.html)
    # wasm options
    target_link_options(miskeenity-canvas PRIVATE -sUSE_WEBGPU=1 -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=[_main,_malloc,_free] -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY)
    # WASM+JS size optimizations
    target_link_options(miskeenity-canvas PRIVATE -sASSERTIONS=0 -sMALLOC=emmalloc )#--closure=1)
else()
    b_embed(miskeenity-canvas ./resources/textures/miskeen_128.png)
    add_dependencies(miskeenity-canvas webgpu_cpp webgpu_dawn onnxruntime_lib)
    target_link_libraries(miskeenity-canvas PRIVATE webgpu_cpp webgpu_dawn onnxruntime_lib)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set_source_files_properties(source/webgpu_surface.c PROPERTIES COMPILE_FLAGS "-x objective-c")
    target_link_libraries(miskeenity-canvas PRIVATE
        "-framework QuartzCore"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework Cocoa"
    )

    # Make the executable look for dylibs in the same directory as the executable
    set_target_properties(miskeenity-canvas PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path"
    )

endif()

# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_link_options(miskeenity-canvas PRIVATE LINKER:-dead_strip)
endif()

# configure installation
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

install(TARGETS miskeenity-canvas COMPONENT miskeenity-canvas RUNTIME DESTINATION bin)
