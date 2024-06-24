#pragma once

#include "app.h"

#include <webgpu/webgpu_cpp.h>

namespace mc
{
    void initMainPipeline( mc::AppContext* app );
    void configureSurface( mc::AppContext* app );
    void uploadTexture( const wgpu::Queue& queue, const wgpu::Texture& texture, void* data, int width, int height, int channels );
    wgpu::Device requestDevice( const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor* descriptor );
    wgpu::Adapter requestAdapter( const wgpu::Instance& instance, const wgpu::RequestAdapterOptions* options );
} // namespace mc