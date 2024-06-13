#pragma once

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

    class TextureManager
    {
      public:
        TextureManager( size_t maxTextures );
        ~TextureManager() = default;

        bool add( void* imageBuffer, int width, int height, int channels, const wgpu::Device& device );
        // bool remove( int index );
        bool bind( int texHandle, int bindGroup, const wgpu::RenderPassEncoder& encoder );

        size_t length() const;

      private:
        void init( const wgpu::Device& device );

        wgpu::Sampler m_sampler;
        wgpu::BindGroupLayout m_groupLayout;

        size_t m_maxLength;
        size_t m_curLength;

        std::unique_ptr<Texture[]> m_array;

        wgpu::Texture m_defaultTexture;
        wgpu::TextureView m_defaultTextureView;
        wgpu::BindGroup m_defaultBindGroup;
    };
} // namespace mc
