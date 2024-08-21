
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG e75175129f83c3d1e85572d03ec070177de8abc4
)
set(BUILD_SHARED_LIBS FALSE)
FetchContent_MakeAvailable(SDL3)

add_definitions(-DSDL_LEAN_AND_MEAN )