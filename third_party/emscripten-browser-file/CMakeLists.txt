
include(FetchContent)

FetchContent_Declare(
    emscripten-browser-file
    GIT_REPOSITORY https://github.com/Armchair-Software/emscripten-browser-file.git
    GIT_TAG 2ff8f8f791c0e7177b40e621ee3f758e03b31bf5
)
FetchContent_GetProperties(emscripten-browser-file)

if(NOT emscripten-browser-file_POPULATED)
  FetchContent_Populate(emscripten-browser-file)
  
  set(emscripten-browser-file_INCLUDE_DIR ${emscripten-browser-file_SOURCE_DIR}/)
  
  add_library(emscripten-browser-file INTERFACE 
                ${emscripten-browser-file_SOURCE_DIR}/emscripten_browser_file.h
  )
  
  target_include_directories(emscripten-browser-file INTERFACE ${emscripten-browser-file_INCLUDE_DIR})

endif()