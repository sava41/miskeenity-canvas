#include "texture_manager.h"

#include "graphics.h"

#include <array>

namespace mc
{

    TextureManager::TextureManager( size_t maxTextures )
        : m_curLength( 0 )
        , m_maxLength( maxTextures )
        , m_array( std::make_unique<Texture[]>( maxTextures ) )
    {
    }

    void TextureManager::init( const wgpu::Device& device )
    {
        m_groupLayout = createTextureBindGroupLayout( device );

        wgpu::SamplerDescriptor samplerDesc;
        samplerDesc.addressModeU  = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeV  = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeW  = wgpu::AddressMode::Repeat;
        samplerDesc.magFilter     = wgpu::FilterMode::Linear;
        samplerDesc.minFilter     = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter  = wgpu::MipmapFilterMode::Linear;
        samplerDesc.lodMinClamp   = 0.0f;
        samplerDesc.lodMaxClamp   = 8.0f;
        samplerDesc.compare       = wgpu::CompareFunction::Undefined;
        samplerDesc.maxAnisotropy = 1;
        m_sampler                 = device.CreateSampler( &samplerDesc );

        // create default white texture
        uint8_t white[4] = { 255, 255, 255, 255 };

        wgpu::TextureDescriptor textureDesc;
        textureDesc.dimension       = wgpu::TextureDimension::e2D;
        textureDesc.format          = wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.size            = { 1, 1, 1 };
        textureDesc.mipLevelCount   = 1;
        textureDesc.sampleCount     = 1;
        textureDesc.usage           = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDesc.viewFormatCount = 0;
        textureDesc.viewFormats     = nullptr;
        m_defaultTexture            = device.CreateTexture( &textureDesc );

        wgpu::TextureViewDescriptor textureViewDesc;
        textureViewDesc.aspect          = wgpu::TextureAspect::All;
        textureViewDesc.baseArrayLayer  = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.baseMipLevel    = 0;
        textureViewDesc.mipLevelCount   = textureDesc.mipLevelCount;
        textureViewDesc.dimension       = wgpu::TextureViewDimension::e2D;
        textureViewDesc.format          = textureDesc.format;

        m_defaultTextureView = m_defaultTexture.CreateView( &textureViewDesc );

        std::array<wgpu::BindGroupEntry, 2> groupEntries;
        groupEntries[0].binding = 0;
        groupEntries[0].sampler = m_sampler;

        groupEntries[1].binding     = 1;
        groupEntries[1].textureView = m_defaultTextureView;

        wgpu::BindGroupDescriptor mainBindGroupDesc;
        mainBindGroupDesc.layout     = m_groupLayout;
        mainBindGroupDesc.entryCount = static_cast<uint32_t>( groupEntries.size() );
        mainBindGroupDesc.entries    = groupEntries.data();

        m_defaultBindGroup = device.CreateBindGroup( &mainBindGroupDesc );

        uploadTexture( device.GetQueue(), m_defaultTexture, &white, 1, 1, 4 );
    };

    bool TextureManager::add( void* imageBuffer, int width, int height, int channels, const wgpu::Device& device )
    {
        if( !m_defaultBindGroup )
        {
            init( device );
        }

        if( m_curLength == m_maxLength )
        {
            return false;
        }

        wgpu::TextureDescriptor textureDesc;
        textureDesc.dimension        = wgpu::TextureDimension::e2D;
        textureDesc.format           = wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.size             = { (unsigned int)width, (unsigned int)height, 1 };
        textureDesc.mipLevelCount    = 1;
        textureDesc.sampleCount      = 1;
        textureDesc.usage            = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDesc.viewFormatCount  = 0;
        textureDesc.viewFormats      = nullptr;
        m_array[m_curLength].texture = device.CreateTexture( &textureDesc );

        wgpu::TextureViewDescriptor textureViewDesc;
        textureViewDesc.aspect          = wgpu::TextureAspect::All;
        textureViewDesc.baseArrayLayer  = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.baseMipLevel    = 0;
        textureViewDesc.mipLevelCount   = textureDesc.mipLevelCount;
        textureViewDesc.dimension       = wgpu::TextureViewDimension::e2D;
        textureViewDesc.format          = textureDesc.format;

        m_array[m_curLength].textureView = m_array[m_curLength].texture.CreateView( &textureViewDesc );

        std::array<wgpu::BindGroupEntry, 2> groupEntries;
        groupEntries[0].binding = 0;
        groupEntries[0].sampler = m_sampler;

        groupEntries[1].binding     = 1;
        groupEntries[1].textureView = m_array[m_curLength].textureView;

        wgpu::BindGroupDescriptor mainBindGroupDesc;
        mainBindGroupDesc.layout     = m_groupLayout;
        mainBindGroupDesc.entryCount = static_cast<uint32_t>( groupEntries.size() );
        mainBindGroupDesc.entries    = groupEntries.data();

        m_array[m_curLength].bindGroup = device.CreateBindGroup( &mainBindGroupDesc );

        uploadTexture( device.GetQueue(), m_array[m_curLength].texture, imageBuffer, width, height, 4 );

        m_curLength += 1;

        return true;
    }

    bool TextureManager::bind( int texHandle, int bindGroup, const wgpu::RenderPassEncoder& encoder )
    {
        if( texHandle < 0 || texHandle >= m_curLength )
        {
            encoder.SetBindGroup( bindGroup, m_defaultBindGroup );
            return false;
        }

        encoder.SetBindGroup( bindGroup, m_array[texHandle].bindGroup );
        return true;
    }

    size_t TextureManager::length() const
    {
        return m_curLength;
    }
} // namespace mc
