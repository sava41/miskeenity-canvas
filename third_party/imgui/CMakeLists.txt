
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/sava41/imgui.git
    GIT_TAG 052c1c73aa53ccdb6910d3fb9b8841395755728d
)
FetchContent_GetProperties(imgui)

if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  
  set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR}/)
  
  file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp)
  file(GLOB IMGUI_HEADERS ${imgui_SOURCE_DIR}/*.h)
  
  add_library(imgui STATIC
                ${IMGUI_SOURCES}
                ${IMGUI_HEADERS}
                ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h
                ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.h
                ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
  )

  if(APPLE)
    set_source_files_properties(
      ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
      PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
  endif()

  target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
  target_link_libraries(imgui PUBLIC SDL3::SDL3)

  if(NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_compile_definitions(imgui PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    target_link_libraries(imgui PUBLIC webgpu_dawn)
  endif()
endif()