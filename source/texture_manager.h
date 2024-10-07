#pragma once

#include "resource_manager.h"

#include <memory>
#include <webgpu/webgpu_cpp.h>

namespace mc
{


    struct Texture
    {
        wgpu::Texture texture;
        wgpu::TextureView textureView;
        wgpu::BindGroup bindGroup;
    };

    class TextureManager : ResourceManager
    {
      public:
        TextureManager( size_t maxTextures );
        ~TextureManager();

        void init( const wgpu::Device& device );
        ResourceHandle add( void* imageBuffer, int width, int height, int channels, const wgpu::Device& device );

        Texture get( const ResourceHandle& texHandle );
        bool bind( const ResourceHandle& texHandle, int bindGroupIndex, const wgpu::RenderPassEncoder& encoder );

      private:
        virtual void freeResource( int resourceIndex ) override;

        wgpu::Sampler m_sampler;
        wgpu::BindGroupLayout m_groupLayout;

        std::unique_ptr<Texture[]> m_array;

        wgpu::Texture m_defaultTexture;
        wgpu::TextureView m_defaultTextureView;
        wgpu::BindGroup m_defaultBindGroup;
    };
} // namespace mc
