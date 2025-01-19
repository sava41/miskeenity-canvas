#pragma once

#include "app.h"

#include <array>
#include <webgpu/webgpu_cpp.h>

namespace mc
{
    bool initDevice( mc::AppContext* app );
    void initPipelines( mc::AppContext* app );
    void initImageProcessingPipelines( mc::AppContext* app );
    void configureSurface( mc::AppContext* app );
    void updateMeshBuffers( mc::AppContext* app );
    wgpu::BindGroupLayout createTextureBindGroupLayout( const wgpu::Device& device );
    wgpu::BindGroupLayout createReadTextureBindGroupLayout( const wgpu::Device& device );
    wgpu::BindGroupLayout createWriteTextureBindGroupLayout( const wgpu::Device& device );
    wgpu::BindGroup createComputeTextureBindGroup( const wgpu::Device& device, const wgpu::Texture& texture, const wgpu::BindGroupLayout& layout );
    void uploadTexture( const wgpu::Queue& queue, const wgpu::Texture& texture, const void* data, int width, int height, int channels );
    void genMipMaps( const wgpu::Device& device, const wgpu::ComputePipeline& pipeline, const wgpu::Texture& texture );
    wgpu::Buffer downloadTexture( const wgpu::Texture& texture, const wgpu::Device& device, const wgpu::CommandEncoder& encoder, int mipLevel = 0 );
    wgpu::Device requestDevice( const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor* descriptor );
    wgpu::Adapter requestAdapter( const wgpu::Instance& instance, const wgpu::RequestAdapterOptions* options );

    template <size_t N>
    wgpu::RenderPassEncoder createRenderPassEncoder( const wgpu::CommandEncoder& encoder, const std::array<wgpu::TextureView, N>& renderTargets,
                                                     const std::array<wgpu::Color, N>& clearColors )
    {
        std::array<wgpu::RenderPassColorAttachment, N> renderPassColorAttachments;
        for( int i = 0; i < renderTargets.size(); ++i )
        {
            renderPassColorAttachments[i].view       = renderTargets[i];
            renderPassColorAttachments[i].loadOp     = wgpu::LoadOp::Clear;
            renderPassColorAttachments[i].storeOp    = wgpu::StoreOp::Store;
            renderPassColorAttachments[i].clearValue = clearColors[i];
        }

        wgpu::RenderPassDescriptor renderPassDesc;
        renderPassDesc.colorAttachmentCount = renderPassColorAttachments.size();
        renderPassDesc.colorAttachments     = renderPassColorAttachments.data();

        return encoder.BeginRenderPass( &renderPassDesc );
    }

} // namespace mc