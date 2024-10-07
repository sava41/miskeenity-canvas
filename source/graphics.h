#pragma once

#include "app.h"

#include <functional>
#include <webgpu/webgpu_cpp.h>

namespace mc
{
    bool initDevice( mc::AppContext* app );
    void initMainPipeline( mc::AppContext* app );
    void configureSurface( mc::AppContext* app );
    void updateMeshBuffers( mc::AppContext* app );
    wgpu::BindGroupLayout createTextureBindGroupLayout( const wgpu::Device& device );
    wgpu::RenderPassEncoder createRenderPassEncoder( const wgpu::CommandEncoder& encoder, const wgpu::TextureView& renderTarget,
                                                     const wgpu::Color& clearColor );
    void uploadTexture( const wgpu::Queue& queue, const wgpu::Texture& texture, void* data, int width, int height, int channels );
    wgpu::Buffer downloadTexture( const wgpu::Texture& texture, std::function<void( wgpu::MapAsyncStatus status, const char* )>& callback,
                                  const wgpu::Device& device, const wgpu::CommandEncoder& encoder );
    wgpu::Device requestDevice( const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor* descriptor );
    wgpu::Adapter requestAdapter( const wgpu::Instance& instance, const wgpu::RequestAdapterOptions* options );
} // namespace mc