#pragma once

#include <webgpu/webgpu.h>
#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

    WGPUSurface SDL_GetWGPUSurface(WGPUInstance instance, SDL_Window* window);

#ifdef __cplusplus
}
#endif