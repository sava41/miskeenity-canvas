
include(FetchContent)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 013ac3beddff3dbffafd5177e7972067cd2b5083
)
FetchContent_GetProperties(stb)

if(NOT stb_POPULATED)
  FetchContent_Populate(stb)
  
  set(stb_INCLUDE_DIR ${stb_SOURCE_DIR}/)
  
  add_library(stb INTERFACE 
                ${stb_SOURCE_DIR}/stb_image.h
  )
  
  target_include_directories(stb INTERFACE ${stb_INCLUDE_DIR})

endif()