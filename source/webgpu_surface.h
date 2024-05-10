#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu_cpp.h>

wgpu::Surface SDL_GetWGPUSurface(const wgpu::Instance &instance, SDL_Window *window);