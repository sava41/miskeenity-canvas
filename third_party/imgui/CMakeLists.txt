
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/sava41/imgui.git
    GIT_TAG 9941588fc0f869a12efa14969da009d620a34e27
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
  
  target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
  target_link_libraries(imgui PUBLIC SDL3::SDL3 webgpu)
endif()