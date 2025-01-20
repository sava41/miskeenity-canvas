
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG preview-3.1.6
)
set(BUILD_SHARED_LIBS FALSE)
FetchContent_MakeAvailable(SDL3)

add_definitions(-DSDL_LEAN_AND_MEAN )