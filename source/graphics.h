#pragma once

#include "app.h"

#include <webgpu/webgpu_cpp.h>

namespace mc
{
    bool initDevice( mc::AppContext* app );
    void initMainPipeline( mc::AppContext* app );
    void configureSurface( mc::AppContext* app );
    wgpu::BindGroupLayout createTextureBindGroupLayout( const wgpu::Device& device );
    void uploadTexture( const wgpu::Queue& queue, const wgpu::Texture& texture, void* data, int width, int height, int channels );
    wgpu::Device requestDevice( const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor* descriptor );
    wgpu::Adapter requestAdapter( const wgpu::Instance& instance, const wgpu::RequestAdapterOptions* options );
} // namespace mc