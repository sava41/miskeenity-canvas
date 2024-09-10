#include "texture_manager.h"

#include "graphics.h"

#include <array>

namespace mc
{

    TextureManager::TextureManager( size_t maxTextures )
        : ResourceManager( maxTextures )
        , m_array( std::make_unique<Texture[]>( maxTextures ) )
    {
    }

    TextureManager::~TextureManager()
    {
        m_defaultTexture.Destroy();

        for( int i = 0; i < maxLength(); ++i )
        {
            m_array[i].texture.Destroy();
        }
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

    ResourceHandle TextureManager::add( void* imageBuffer, int width, int height, int channels, const wgpu::Device& device )
    {
        if( !m_defaultBindGroup )
        {
            init( device );
        }

        if( curLength() == maxLength() )
        {
            return ResourceHandle::invalidResource();
        }

        if( channels != 1 && channels != 4 )
        {
            return ResourceHandle::invalidResource();
        }

        int textureIndex = 0;
        for( int i = 0; i < maxLength(); ++i )
        {
            if( getRefCount( i ) == 0 )
            {
                textureIndex = i;
                break;
            }
        }

        wgpu::TextureDescriptor textureDesc;
        textureDesc.dimension        = wgpu::TextureDimension::e2D;
        textureDesc.format            = channels == 1 ? wgpu::TextureFormat::R8Unorm : wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.size             = { (unsigned int)width, (unsigned int)height, 1 };
        textureDesc.mipLevelCount    = 1;
        textureDesc.sampleCount      = 1;
        textureDesc.usage            = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDesc.viewFormatCount  = 0;
        textureDesc.viewFormats      = nullptr;
        m_array[textureIndex].texture = device.CreateTexture( &textureDesc );

        wgpu::TextureViewDescriptor textureViewDesc;
        textureViewDesc.aspect          = wgpu::TextureAspect::All;
        textureViewDesc.baseArrayLayer  = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.baseMipLevel    = 0;
        textureViewDesc.mipLevelCount   = textureDesc.mipLevelCount;
        textureViewDesc.dimension       = wgpu::TextureViewDimension::e2D;
        textureViewDesc.format          = textureDesc.format;

        m_array[textureIndex].textureView = m_array[textureIndex].texture.CreateView( &textureViewDesc );

        std::array<wgpu::BindGroupEntry, 2> groupEntries;
        groupEntries[0].binding = 0;
        groupEntries[0].sampler = m_sampler;

        groupEntries[1].binding     = 1;
        groupEntries[1].textureView = m_array[textureIndex].textureView;

        wgpu::BindGroupDescriptor mainBindGroupDesc;
        mainBindGroupDesc.layout     = m_groupLayout;
        mainBindGroupDesc.entryCount = static_cast<uint32_t>( groupEntries.size() );
        mainBindGroupDesc.entries    = groupEntries.data();

        m_array[textureIndex].bindGroup = device.CreateBindGroup( &mainBindGroupDesc );

        uploadTexture( device.GetQueue(), m_array[textureIndex].texture, imageBuffer, width, height, channels );

        return getHandle( textureIndex );
    }

    bool TextureManager::bind( const ResourceHandle& texHandle, int bindGroupIndex, const wgpu::RenderPassEncoder& encoder )
    {
        if( !texHandle.valid() )
        {
            encoder.SetBindGroup( bindGroupIndex, m_defaultBindGroup );
            return false;
        }

        encoder.SetBindGroup( bindGroupIndex, m_array[texHandle.resourceIndex()].bindGroup );
        return true;
    }

    void TextureManager::freeResource( int resourceIndex )
    {
        m_array[resourceIndex].texture.Destroy();
    }
} // namespace mc
