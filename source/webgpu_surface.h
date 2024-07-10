#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif
#define HTML_CANVAS_ID "canvas"

    WGPUSurface SDL_GetWGPUSurface( WGPUInstance instance, SDL_Window* window );

#ifdef __cplusplus
}
#endif