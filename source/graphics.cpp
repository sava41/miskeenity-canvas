#include "graphics.h"

#include "battery/embed.hpp"

#include <array>
#if defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten/emscripten.h>
#endif

namespace mc
{

    bool initDevice( mc::AppContext* app )
    {
        wgpu::RequestAdapterOptions adapterOpts;
        adapterOpts.compatibleSurface = app->surface;

        app->adapter = mc::requestAdapter( app->instance, &adapterOpts );
        if( !app->adapter )
        {
            return false;
        }

        wgpu::Limits supportedLimits;
        app->adapter.GetLimits( &supportedLimits );

        app->maxBufferSize = std::min<uint64_t>( MaxMeshBufferSize, supportedLimits.maxBufferSize );

        wgpu::Limits requiredLimits;
        requiredLimits.maxVertexAttributes = 6;
        requiredLimits.maxVertexBuffers    = 1;
        requiredLimits.maxBufferSize       = app->maxBufferSize;
        // requiredLimits.maxVertexBufferArrayStride = sizeof(WGPUVertexAttributes);
        requiredLimits.maxUniformBufferBindingSize     = mc::NumLayers * sizeof( mc::Layer );
        requiredLimits.minStorageBufferOffsetAlignment = supportedLimits.minStorageBufferOffsetAlignment;
        requiredLimits.minUniformBufferOffsetAlignment = supportedLimits.minUniformBufferOffsetAlignment;
        requiredLimits.maxInterStageShaderVariables    = 8;
        requiredLimits.maxBindGroups                   = 4;
        requiredLimits.maxUniformBuffersPerShaderStage = 1;
        requiredLimits.maxUniformBufferBindingSize     = 16 * 4 * sizeof( float );
        // Allow textures up to 2K
        requiredLimits.maxTextureDimension1D            = 2048;
        requiredLimits.maxTextureDimension2D            = 2048;
        requiredLimits.maxTextureArrayLayers            = 1;
        requiredLimits.maxSampledTexturesPerShaderStage = 1;
        requiredLimits.maxSamplersPerShaderStage        = 1;

        wgpu::DeviceDescriptor deviceDesc;
        deviceDesc.label              = "Device";
        deviceDesc.requiredLimits     = &requiredLimits;
        deviceDesc.defaultQueue.label = "Main Queue";
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        deviceDesc.SetUncapturedErrorCallback(
            []( const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message, mc::AppContext* ctx )
            {
                SDL_Log( "Device error type: %d\n", static_cast<int>( type ) );
                SDL_Log( "Device error message: %.*s\n", static_cast<int>( message.length ), message.data );
                ctx->appQuit = true;
            },
            app );
#endif
        app->device = mc::requestDevice( app->adapter, &deviceDesc );

        wgpu::SurfaceCapabilities capabilities;
        app->surface.GetCapabilities( app->adapter, &capabilities );
        app->colorFormat = capabilities.formats[0];

        return true;
    }

    void initPipelines( mc::AppContext* app )
    {
        // we have one vertex buffers
        // the vertex buffer is populated in the mesh assember compute shader
        std::array<wgpu::VertexBufferLayout, 1> vertexBufLayout;

        // we have 5 vertex attributes, 2d position, uv, size, color, layer id
        std::array<wgpu::VertexAttribute, 5> vertexAttr;
        vertexAttr[0].format         = wgpu::VertexFormat::Float32x2;
        vertexAttr[0].offset         = 0;
        vertexAttr[0].shaderLocation = 0;

        vertexAttr[1].format         = wgpu::VertexFormat::Float32x2;
        vertexAttr[1].offset         = 2 * sizeof( float );
        vertexAttr[1].shaderLocation = 1;

        vertexAttr[2].format         = wgpu::VertexFormat::Float32x2;
        vertexAttr[2].offset         = 2 * sizeof( float ) + 2 * sizeof( float );
        vertexAttr[2].shaderLocation = 2;

        vertexAttr[3].format         = wgpu::VertexFormat::Unorm8x4;
        vertexAttr[3].offset         = 2 * sizeof( float ) + 2 * sizeof( float ) + 2 * sizeof( float );
        vertexAttr[3].shaderLocation = 3;

        vertexAttr[4].format         = wgpu::VertexFormat::Uint32;
        vertexAttr[4].offset         = 2 * sizeof( float ) + 2 * sizeof( float ) + 2 * sizeof( float ) + sizeof( float );
        vertexAttr[4].shaderLocation = 4;

        vertexBufLayout[0].arrayStride    = 8 * sizeof( float );
        vertexBufLayout[0].attributeCount = static_cast<uint32_t>( vertexAttr.size() );
        vertexBufLayout[0].attributes     = vertexAttr.data();

        wgpu::ShaderSourceWGSL shaderCodeDesc;
        shaderCodeDesc.code = b::embed<"./resources/shaders/layers.wgsl">().data();

        wgpu::ShaderModuleDescriptor shaderDesc;
        shaderDesc.nextInChain = &shaderCodeDesc;

        wgpu::ShaderModule shaderModule = app->device.CreateShaderModule( &shaderDesc );

        wgpu::BlendState blendState;
        wgpu::BlendState maskBlendState;

        blendState.color.srcFactor = wgpu::BlendFactor::One;
        blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blendState.color.operation = wgpu::BlendOperation::Add;

        blendState.alpha.srcFactor = wgpu::BlendFactor::One;
        blendState.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blendState.alpha.operation = wgpu::BlendOperation::Add;

        maskBlendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        maskBlendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        maskBlendState.color.operation = wgpu::BlendOperation::Add;

        maskBlendState.alpha.srcFactor = wgpu::BlendFactor::One;
        maskBlendState.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        maskBlendState.alpha.operation = wgpu::BlendOperation::Add;

        std::array<wgpu::ColorTargetState, 3> mainRenderTargets;

        mainRenderTargets[0].format    = wgpu::TextureFormat::RGBA8Unorm;
        mainRenderTargets[0].blend     = &blendState;
        mainRenderTargets[0].writeMask = wgpu::ColorWriteMask::All;

        mainRenderTargets[1].format    = wgpu::TextureFormat::R8Unorm;
        mainRenderTargets[1].blend     = &maskBlendState;
        mainRenderTargets[1].writeMask = wgpu::ColorWriteMask::Red;

        mainRenderTargets[2].format    = wgpu::TextureFormat::R8Unorm;
        mainRenderTargets[2].blend     = &maskBlendState;
        mainRenderTargets[2].writeMask = wgpu::ColorWriteMask::Red;

        wgpu::FragmentState fragmentState;
        fragmentState.module        = shaderModule;
        fragmentState.entryPoint    = "fs_main";
        fragmentState.constantCount = 0;
        fragmentState.targetCount   = mainRenderTargets.size();
        fragmentState.targets       = mainRenderTargets.data();

        wgpu::VertexState vertexState;
        vertexState.module        = shaderModule;
        vertexState.entryPoint    = "vs_main";
        vertexState.bufferCount   = static_cast<uint32_t>( vertexBufLayout.size() );
        vertexState.constantCount = 0;
        vertexState.buffers       = vertexBufLayout.data();

        // Create global bind group layout
        std::array<wgpu::BindGroupLayoutEntry, 2> globalGroupLayoutEntries;
        globalGroupLayoutEntries[0].binding                 = 0;
        globalGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
        globalGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
        globalGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::Uniform;
        globalGroupLayoutEntries[0].buffer.minBindingSize   = sizeof( mc::Uniforms );

        globalGroupLayoutEntries[1].binding                 = 1;
        globalGroupLayoutEntries[1].visibility              = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
        globalGroupLayoutEntries[1].buffer.hasDynamicOffset = false;
        globalGroupLayoutEntries[1].buffer.type             = wgpu::BufferBindingType::ReadOnlyStorage;
        globalGroupLayoutEntries[1].buffer.minBindingSize   = sizeof( mc::Layer );

        wgpu::BindGroupLayoutDescriptor globalGroupLayoutDesc;
        globalGroupLayoutDesc.entryCount = static_cast<uint32_t>( globalGroupLayoutEntries.size() );
        globalGroupLayoutDesc.entries    = globalGroupLayoutEntries.data();

        wgpu::BindGroupLayout globalGroupLayout = app->device.CreateBindGroupLayout( &globalGroupLayoutDesc );

        // Create mesh data bind group layout
        std::array<wgpu::BindGroupLayoutEntry, 2> meshGroupLayoutEntries;
        meshGroupLayoutEntries[0].binding                 = 0;
        meshGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Compute;
        meshGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
        meshGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::ReadOnlyStorage;

        meshGroupLayoutEntries[1].binding                 = 1;
        meshGroupLayoutEntries[1].visibility              = wgpu::ShaderStage::Compute;
        meshGroupLayoutEntries[1].buffer.hasDynamicOffset = false;
        meshGroupLayoutEntries[1].buffer.type             = wgpu::BufferBindingType::Storage;

        wgpu::BindGroupLayoutDescriptor meshBindGroupLayoutDesc;
        meshBindGroupLayoutDesc.entryCount = static_cast<uint32_t>( meshGroupLayoutEntries.size() );
        meshBindGroupLayoutDesc.entries    = meshGroupLayoutEntries.data();

        wgpu::BindGroupLayout meshGroupLayout = app->device.CreateBindGroupLayout( &meshBindGroupLayoutDesc );

        // Create texture bind group layout for main pipelinee
        wgpu::BindGroupLayout textureGroupLayout = createTextureBindGroupLayout( app->device );

        // Create main pipeline
        std::array<wgpu::BindGroupLayout, 3> mainBindGroupLayouts = { globalGroupLayout, textureGroupLayout, textureGroupLayout };

        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>( mainBindGroupLayouts.size() );
        pipelineLayoutDesc.bindGroupLayouts     = mainBindGroupLayouts.data();

        wgpu::PipelineLayout pipelineLayout = app->device.CreatePipelineLayout( &pipelineLayoutDesc );

        wgpu::RenderPipelineDescriptor renderPipelineDesc;
        renderPipelineDesc.label                              = "Canvas";
        renderPipelineDesc.vertex                             = vertexState;
        renderPipelineDesc.fragment                           = &fragmentState;
        renderPipelineDesc.layout                             = pipelineLayout;
        renderPipelineDesc.primitive.topology                 = wgpu::PrimitiveTopology::TriangleList;
        renderPipelineDesc.primitive.stripIndexFormat         = wgpu::IndexFormat::Undefined;
        renderPipelineDesc.primitive.frontFace                = wgpu::FrontFace::CCW;
        renderPipelineDesc.multisample.count                  = 1;
        renderPipelineDesc.multisample.mask                   = ~0u;
        renderPipelineDesc.multisample.alphaToCoverageEnabled = false;

        app->canvasPipeline = app->device.CreateRenderPipeline( &renderPipelineDesc );

        // export pipeline is the exact same as canvas but we only need the main color render target
        fragmentState.targetCount = 1;
        renderPipelineDesc.label  = "Export";

        app->exportPipeline = app->device.CreateRenderPipeline( &renderPipelineDesc );

        // Create buffers
        wgpu::BufferDescriptor uboBufDesc;
        uboBufDesc.mappedAtCreation = false;
        uboBufDesc.size             = sizeof( mc::Uniforms );
        uboBufDesc.usage            = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        app->viewParamBuf           = app->device.CreateBuffer( &uboBufDesc );

        wgpu::BufferDescriptor layerBufDesc;
        layerBufDesc.mappedAtCreation = false;
        layerBufDesc.size             = mc::NumLayers * sizeof( mc::Layer );
        layerBufDesc.usage            = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        app->layerBuf                 = app->device.CreateBuffer( &layerBufDesc );

        wgpu::BufferDescriptor selectionOutputBufDesc;
        selectionOutputBufDesc.mappedAtCreation = false;
        selectionOutputBufDesc.size             = app->maxBufferSize / sizeof( mc::Triangle ) * sizeof( mc::Selection );
        selectionOutputBufDesc.usage            = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        app->selectionBuf                       = app->device.CreateBuffer( &selectionOutputBufDesc );

        selectionOutputBufDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
        app->selectionMapBuf         = app->device.CreateBuffer( &selectionOutputBufDesc );

        // Create the bind group for the global data
        std::array<wgpu::BindGroupEntry, 2> globalGroupEntries;
        globalGroupEntries[0].binding = 0;
        globalGroupEntries[0].buffer  = app->viewParamBuf;
        globalGroupEntries[0].size    = app->viewParamBuf.GetSize();

        globalGroupEntries[1].binding = 1;
        globalGroupEntries[1].buffer  = app->layerBuf;
        globalGroupEntries[1].size    = app->layerBuf.GetSize();

        wgpu::BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout     = globalGroupLayout;
        bindGroupDesc.entryCount = static_cast<uint32_t>( globalGroupEntries.size() );
        bindGroupDesc.entries    = globalGroupEntries.data();

        app->globalBindGroup = app->device.CreateBindGroup( &bindGroupDesc );

        wgpu::BufferDescriptor vertexBufferDesc;
        vertexBufferDesc.mappedAtCreation = false;
        vertexBufferDesc.size             = app->maxBufferSize;
        vertexBufferDesc.usage            = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
        app->vertexBuf                    = app->device.CreateBuffer( &vertexBufferDesc );

        vertexBufferDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
        app->vertexCopyBuf     = app->device.CreateBuffer( &vertexBufferDesc );

        // Set up post process pipeline
        {
            wgpu::ShaderSourceWGSL postShaderCodeDesc;
            postShaderCodeDesc.code = b::embed<"./resources/shaders/postprocess.wgsl">().data();

            wgpu::ShaderModuleDescriptor postShaderDesc;
            postShaderDesc.nextInChain = &postShaderCodeDesc;

            wgpu::ShaderModule postShaderModule = app->device.CreateShaderModule( &postShaderDesc );

            wgpu::ColorTargetState postColorTarget;
            postColorTarget.format    = app->colorFormat;
            postColorTarget.blend     = &blendState;
            postColorTarget.writeMask = wgpu::ColorWriteMask::All;

            wgpu::FragmentState postFragmentState;
            postFragmentState.module        = postShaderModule;
            postFragmentState.entryPoint    = "fs_post";
            postFragmentState.constantCount = 0;
            postFragmentState.targetCount   = 1;
            postFragmentState.targets       = &postColorTarget;

            wgpu::VertexState postVertexState;
            postVertexState.module        = postShaderModule;
            postVertexState.entryPoint    = "vs_post";
            postVertexState.bufferCount   = 0;
            postVertexState.constantCount = 0;

            std::array<wgpu::BindGroupLayout, 4> postBindGroupLayouts = { globalGroupLayout, textureGroupLayout, textureGroupLayout, textureGroupLayout };

            wgpu::PipelineLayoutDescriptor postPipelineLayoutDesc;
            postPipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>( postBindGroupLayouts.size() );
            postPipelineLayoutDesc.bindGroupLayouts     = postBindGroupLayouts.data();

            wgpu::PipelineLayout postPipelineLayout = app->device.CreatePipelineLayout( &postPipelineLayoutDesc );

            wgpu::RenderPipelineDescriptor postPipelineDesc;
            postPipelineDesc.label                              = "Post Process";
            postPipelineDesc.vertex                             = postVertexState;
            postPipelineDesc.fragment                           = &postFragmentState;
            postPipelineDesc.layout                             = postPipelineLayout;
            postPipelineDesc.primitive.topology                 = wgpu::PrimitiveTopology::TriangleList;
            postPipelineDesc.primitive.stripIndexFormat         = wgpu::IndexFormat::Undefined;
            postPipelineDesc.primitive.frontFace                = wgpu::FrontFace::CCW;
            postPipelineDesc.multisample.count                  = 1;
            postPipelineDesc.multisample.mask                   = ~0u;
            postPipelineDesc.multisample.alphaToCoverageEnabled = false;

            app->postPipeline = app->device.CreateRenderPipeline( &postPipelineDesc );
        }

        // Set up compute shader used to compute selection
        {
            wgpu::ShaderSourceWGSL selectionShaderCodeDesc;
            selectionShaderCodeDesc.code = b::embed<"./resources/shaders/selection.wgsl">().data();

            wgpu::ShaderModuleDescriptor selectionShaderModuleDesc;
            selectionShaderModuleDesc.nextInChain = &selectionShaderCodeDesc;

            wgpu::ShaderModule selectionShaderModule = app->device.CreateShaderModule( &selectionShaderModuleDesc );

            std::array<wgpu::BindGroupLayoutEntry, 1> selectionGroupLayoutEntries;

            selectionGroupLayoutEntries[0].binding                 = 0;
            selectionGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Compute;
            selectionGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
            selectionGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::Storage;
            selectionGroupLayoutEntries[0].buffer.minBindingSize   = sizeof( mc::Selection );

            wgpu::BindGroupLayoutDescriptor selectionBindGroupLayoutDesc;
            selectionBindGroupLayoutDesc.entryCount = static_cast<uint32_t>( selectionGroupLayoutEntries.size() );
            selectionBindGroupLayoutDesc.entries    = selectionGroupLayoutEntries.data();

            wgpu::BindGroupLayout selectionGroupLayout = app->device.CreateBindGroupLayout( &selectionBindGroupLayoutDesc );

            std::array<wgpu::BindGroupLayout, 3> selectionBindGroupLayouts = { globalGroupLayout, meshGroupLayout, selectionGroupLayout };

            wgpu::PipelineLayoutDescriptor selectionPipelineLayoutDesc;
            selectionPipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>( selectionBindGroupLayouts.size() );
            selectionPipelineLayoutDesc.bindGroupLayouts     = selectionBindGroupLayouts.data();

            wgpu::PipelineLayout selectionPipelineLayout = app->device.CreatePipelineLayout( &selectionPipelineLayoutDesc );

            wgpu::ComputePipelineDescriptor selectionPipelineDesc;
            selectionPipelineDesc.label              = "Canvas Selection";
            selectionPipelineDesc.layout             = selectionPipelineLayout;
            selectionPipelineDesc.compute.module     = selectionShaderModule;
            selectionPipelineDesc.compute.entryPoint = "cs_main";

            app->selectionPipeline = app->device.CreateComputePipeline( &selectionPipelineDesc );

            std::array<wgpu::BindGroupEntry, 1> selectionGroupEntries;

            selectionGroupEntries[0].binding = 0;
            selectionGroupEntries[0].buffer  = app->selectionBuf;
            selectionGroupEntries[0].offset  = 0;
            selectionGroupEntries[0].size    = app->selectionBuf.GetSize();

            wgpu::BindGroupDescriptor selectionBindGroupDesc;
            selectionBindGroupDesc.layout     = selectionGroupLayout;
            selectionBindGroupDesc.entryCount = static_cast<uint32_t>( selectionGroupEntries.size() );
            selectionBindGroupDesc.entries    = selectionGroupEntries.data();

            app->selectionBindGroup = app->device.CreateBindGroup( &selectionBindGroupDesc );
        }

        // Set up compute shader used to assemble the vertex buffer
        {

            wgpu::ShaderSourceWGSL meshShaderCodeDesc;
            meshShaderCodeDesc.code = b::embed<"./resources/shaders/mesh.wgsl">().data();

            wgpu::ShaderModuleDescriptor meshShaderModuleDesc;
            meshShaderModuleDesc.nextInChain = &meshShaderCodeDesc;

            wgpu::ShaderModule meshShaderModule = app->device.CreateShaderModule( &meshShaderModuleDesc );

            std::array<wgpu::BindGroupLayout, 2> meshBindGroupLayouts = { globalGroupLayout, meshGroupLayout };

            wgpu::PipelineLayoutDescriptor meshPipelineLayoutDesc;
            meshPipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>( meshBindGroupLayouts.size() );
            meshPipelineLayoutDesc.bindGroupLayouts     = meshBindGroupLayouts.data();

            wgpu::PipelineLayout meshPipelineLayout = app->device.CreatePipelineLayout( &meshPipelineLayoutDesc );


            wgpu::ComputePipelineDescriptor meshPipelineDesc;
            meshPipelineDesc.label              = "Mesh Assembler";
            meshPipelineDesc.layout             = meshPipelineLayout;
            meshPipelineDesc.compute.module     = meshShaderModule;
            meshPipelineDesc.compute.entryPoint = "ma_main";

            app->meshPipeline = app->device.CreateComputePipeline( &meshPipelineDesc );
        }
    }

    void initImageProcessingPipelines( mc::AppContext* app )
    {
        wgpu::BindGroupLayout readGroupLayout  = createReadTextureBindGroupLayout( app->device );
        wgpu::BindGroupLayout writeGroupLayout = createWriteTextureBindGroupLayout( app->device );

        wgpu::ShaderSourceWGSL preAlphaShaderCodeDesc;
        preAlphaShaderCodeDesc.code = b::embed<"./resources/shaders/prealpha.wgsl">().data();

        wgpu::ShaderModuleDescriptor preAlphaShaderModuleDesc;
        preAlphaShaderModuleDesc.nextInChain = &preAlphaShaderCodeDesc;

        wgpu::ShaderModule preAlphaShaderModule = app->device.CreateShaderModule( &preAlphaShaderModuleDesc );

        std::array<wgpu::BindGroupLayout, 2> readWriteBindGroupLayouts = { readGroupLayout, writeGroupLayout };

        wgpu::PipelineLayoutDescriptor readWritePipelineLayoutDesc;
        readWritePipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>( readWriteBindGroupLayouts.size() );
        readWritePipelineLayoutDesc.bindGroupLayouts     = readWriteBindGroupLayouts.data();

        wgpu::PipelineLayout readWritePipelineLayout = app->device.CreatePipelineLayout( &readWritePipelineLayoutDesc );

        wgpu::ComputePipelineDescriptor pipelineDesc;
        pipelineDesc.label              = "Premultiplied Alpha";
        pipelineDesc.layout             = readWritePipelineLayout;
        pipelineDesc.compute.module     = preAlphaShaderModule;
        pipelineDesc.compute.entryPoint = "pre_alpha";

        app->preAlphaPipeline = app->device.CreateComputePipeline( &pipelineDesc );

        wgpu::ShaderSourceWGSL mipGenShaderCodeDesc;
        mipGenShaderCodeDesc.code = b::embed<"./resources/shaders/mipgen.wgsl">().data();

        wgpu::ShaderModuleDescriptor mipGenShaderModuleDesc;
        mipGenShaderModuleDesc.nextInChain = &mipGenShaderCodeDesc;

        wgpu::ShaderModule mipGenShaderModule = app->device.CreateShaderModule( &mipGenShaderModuleDesc );

        pipelineDesc.label              = "Compute Mip";
        pipelineDesc.layout             = readWritePipelineLayout;
        pipelineDesc.compute.module     = mipGenShaderModule;
        pipelineDesc.compute.entryPoint = "compute_mip";

        app->mipGenPipeline = app->device.CreateComputePipeline( &pipelineDesc );

        wgpu::ShaderSourceWGSL maskMutiplyShaderCodeDesc;
        maskMutiplyShaderCodeDesc.code = b::embed<"./resources/shaders/maskmultiply.wgsl">().data();

        wgpu::ShaderModuleDescriptor maskMutiplyShaderModuleDesc;
        maskMutiplyShaderModuleDesc.nextInChain = &maskMutiplyShaderCodeDesc;

        wgpu::ShaderModule maskMutiplyShaderModule = app->device.CreateShaderModule( &maskMutiplyShaderModuleDesc );

        wgpu::BindGroupLayout readSamplerGroupLayout = createTextureBindGroupLayout( app->device );

        std::array<wgpu::BindGroupLayout, 3> readReadWriteBindGroupLayouts = { readSamplerGroupLayout, readSamplerGroupLayout, writeGroupLayout };

        wgpu::PipelineLayoutDescriptor readReadWritePipelineLayoutDesc;
        readReadWritePipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>( readReadWriteBindGroupLayouts.size() );
        readReadWritePipelineLayoutDesc.bindGroupLayouts     = readReadWriteBindGroupLayouts.data();

        wgpu::PipelineLayout readReadWritePipelineLayout = app->device.CreatePipelineLayout( &readReadWritePipelineLayoutDesc );

        wgpu::ComputePipelineDescriptor maskMultiplyPipelineDesc;
        maskMultiplyPipelineDesc.label              = "Mask Mutiply";
        maskMultiplyPipelineDesc.layout             = readReadWritePipelineLayout;
        maskMultiplyPipelineDesc.compute.module     = maskMutiplyShaderModule;
        maskMultiplyPipelineDesc.compute.entryPoint = "mask_multipy";

        app->maskMultiplyPipeline = app->device.CreateComputePipeline( &maskMultiplyPipelineDesc );

        maskMultiplyPipelineDesc.label              = "Inverse Mask Mutiply";
        maskMultiplyPipelineDesc.compute.entryPoint = "inv_mask_multipy";

        app->invMaskMultiplyPipeline = app->device.CreateComputePipeline( &maskMultiplyPipelineDesc );
    }

    void configureSurface( mc::AppContext* app )
    {
        wgpu::SurfaceConfiguration config;
        config.device = app->device;
        config.format = app->colorFormat;
        config.width  = app->bbwidth;
        config.height = app->bbheight;
        app->surface.Configure( &config );
    }

    void updateMeshBuffers( mc::AppContext* app )
    {
        if( app->meshBuf == nullptr || app->meshBuf.GetSize() != app->meshManager.size() )
        {
            if( app->meshBuf )
            {
                app->meshBuf.Destroy();
            }

            wgpu::BufferDescriptor meshBufDesc;
            meshBufDesc.mappedAtCreation = true;
            meshBufDesc.size             = app->meshManager.size();
            meshBufDesc.usage            = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
            app->meshBuf                 = app->device.CreateBuffer( &meshBufDesc );

            std::memcpy( app->meshBuf.GetMappedRange(), app->meshManager.data(), app->meshManager.size() );
            app->meshBuf.Unmap();
        }

        std::array<wgpu::BindGroupEntry, 2> meshGroupEntries;

        meshGroupEntries[0].binding = 0;
        meshGroupEntries[0].buffer  = app->meshBuf;
        meshGroupEntries[0].offset  = 0;
        meshGroupEntries[0].size    = app->meshBuf.GetSize();

        meshGroupEntries[1].binding = 1;
        meshGroupEntries[1].buffer  = app->vertexBuf;
        meshGroupEntries[1].offset  = 0;
        meshGroupEntries[1].size    = app->vertexBuf.GetSize();

        wgpu::BindGroupDescriptor meshBindGroupDesc;
        meshBindGroupDesc.layout     = app->meshPipeline.GetBindGroupLayout( 1 );
        meshBindGroupDesc.entryCount = static_cast<uint32_t>( meshGroupEntries.size() );
        meshBindGroupDesc.entries    = meshGroupEntries.data();

        app->meshBindGroup = app->device.CreateBindGroup( &meshBindGroupDesc );
    }

    wgpu::BindGroupLayout createTextureBindGroupLayout( const wgpu::Device& device )
    {
        std::array<wgpu::BindGroupLayoutEntry, 2> groupLayoutEntries;

        groupLayoutEntries[0].binding      = 0;
        groupLayoutEntries[0].visibility   = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
        groupLayoutEntries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

        groupLayoutEntries[1].binding               = 1;
        groupLayoutEntries[1].visibility            = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
        groupLayoutEntries[1].texture.sampleType    = wgpu::TextureSampleType::Float;
        groupLayoutEntries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        wgpu::BindGroupLayoutDescriptor groupLayoutDesc;
        groupLayoutDesc.entryCount = static_cast<uint32_t>( groupLayoutEntries.size() );
        groupLayoutDesc.entries    = groupLayoutEntries.data();

        return device.CreateBindGroupLayout( &groupLayoutDesc );
    }

    wgpu::BindGroupLayout createReadTextureBindGroupLayout( const wgpu::Device& device )
    {
        wgpu::BindGroupLayoutEntry readBindGroupLayoutEntry;
        readBindGroupLayoutEntry.binding               = 0;
        readBindGroupLayoutEntry.visibility            = wgpu::ShaderStage::Compute;
        readBindGroupLayoutEntry.texture.sampleType    = wgpu::TextureSampleType::Float;
        readBindGroupLayoutEntry.texture.viewDimension = wgpu::TextureViewDimension::e2D;

        wgpu::BindGroupLayoutDescriptor readBindGroupLayoutDesc;
        readBindGroupLayoutDesc.entryCount = 1;
        readBindGroupLayoutDesc.entries    = &readBindGroupLayoutEntry;

        return device.CreateBindGroupLayout( &readBindGroupLayoutDesc );
    }

    wgpu::BindGroupLayout createWriteTextureBindGroupLayout( const wgpu::Device& device )
    {
        wgpu::BindGroupLayoutEntry writeBindGroupLayoutEntry;
        writeBindGroupLayoutEntry.binding                      = 0;
        writeBindGroupLayoutEntry.visibility                   = wgpu::ShaderStage::Compute;
        writeBindGroupLayoutEntry.storageTexture.access        = wgpu::StorageTextureAccess::WriteOnly;
        writeBindGroupLayoutEntry.storageTexture.format        = wgpu::TextureFormat::RGBA8Unorm;
        writeBindGroupLayoutEntry.storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

        wgpu::BindGroupLayoutDescriptor writeBindGroupLayoutDesc;
        writeBindGroupLayoutDesc.entryCount = 1;
        writeBindGroupLayoutDesc.entries    = &writeBindGroupLayoutEntry;

        return device.CreateBindGroupLayout( &writeBindGroupLayoutDesc );
    }

    wgpu::BindGroup createComputeTextureBindGroup( const wgpu::Device& device, const wgpu::Texture& texture, const wgpu::BindGroupLayout& layout )
    {
        wgpu::TextureViewDescriptor textureViewDesc;
        textureViewDesc.aspect          = wgpu::TextureAspect::All;
        textureViewDesc.baseArrayLayer  = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.baseMipLevel    = 0;
        textureViewDesc.mipLevelCount   = 1;
        textureViewDesc.dimension       = wgpu::TextureViewDimension::e2D;
        textureViewDesc.format          = texture.GetFormat();

        wgpu::TextureView textureView = texture.CreateView( &textureViewDesc );

        wgpu::BindGroupEntry bindGroupEntry;
        bindGroupEntry.binding     = 0;
        bindGroupEntry.textureView = textureView;

        wgpu::BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout     = layout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries    = &bindGroupEntry;

        return device.CreateBindGroup( &bindGroupDesc );
    }

    void uploadTexture( const wgpu::Queue& queue, const wgpu::Texture& texture, const void* data, int width, int height, int channels )
    {
        wgpu::TexelCopyTextureInfo imageCopyTexture;
        imageCopyTexture.texture  = texture;
        imageCopyTexture.mipLevel = 0;
        imageCopyTexture.origin   = { 0, 0, 0 };
        imageCopyTexture.aspect   = wgpu::TextureAspect::All;

        wgpu::TexelCopyBufferLayout textureDataLayout;
        textureDataLayout.offset       = 0;
        textureDataLayout.bytesPerRow  = width * channels;
        textureDataLayout.rowsPerImage = height;

        wgpu::Extent3D writeSize;
        writeSize.width              = width;
        writeSize.height             = height;
        writeSize.depthOrArrayLayers = 1;

        queue.WriteTexture( &imageCopyTexture, data, width * height * channels, &textureDataLayout, &writeSize );
    }

    void genMipMaps( const wgpu::Device& device, const wgpu::ComputePipeline& pipeline, const wgpu::Texture& texture )
    {
        if( texture.GetMipLevelCount() == 1 )
        {
            return;
        }

        wgpu::TextureViewDescriptor textureViewDesc;
        textureViewDesc.aspect          = wgpu::TextureAspect::All;
        textureViewDesc.baseArrayLayer  = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.dimension       = wgpu::TextureViewDimension::e2D;
        textureViewDesc.format          = wgpu::TextureFormat::RGBA8Unorm;
        textureViewDesc.mipLevelCount   = 1;
        textureViewDesc.baseMipLevel    = 0;

        wgpu::TextureView prevView = texture.CreateView( &textureViewDesc );

        uint32_t prevWidth  = texture.GetWidth();
        uint32_t prevHeight = texture.GetHeight();

        wgpu::CommandEncoderDescriptor commandEncoderDesc;
        commandEncoderDesc.label = "Mip Maps";

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder( &commandEncoderDesc );

        wgpu::ComputePassEncoder computePassEnc = encoder.BeginComputePass();
        computePassEnc.SetPipeline( pipeline );

        for( int i = 1; i < texture.GetMipLevelCount(); ++i )
        {
            textureViewDesc.baseMipLevel = i;
            wgpu::TextureView currView   = texture.CreateView( &textureViewDesc );

            wgpu::BindGroupEntry bindGroupEntry;
            bindGroupEntry.binding     = 0;
            bindGroupEntry.textureView = prevView;

            wgpu::BindGroupDescriptor bindGroupDesc;
            bindGroupDesc.layout     = createReadTextureBindGroupLayout( device );
            bindGroupDesc.entryCount = 1;
            bindGroupDesc.entries    = &bindGroupEntry;

            wgpu::BindGroup inputBindGroup = device.CreateBindGroup( &bindGroupDesc );

            bindGroupEntry.textureView = currView;

            bindGroupDesc.layout = createWriteTextureBindGroupLayout( device );

            wgpu::BindGroup outputBindGroup = device.CreateBindGroup( &bindGroupDesc );

            uint32_t currWidth  = prevWidth / 2;
            uint32_t currHeight = prevHeight / 2;

            computePassEnc.SetBindGroup( 0, inputBindGroup );
            computePassEnc.SetBindGroup( 1, outputBindGroup );

            computePassEnc.DispatchWorkgroups( ( currWidth + 8 - 1 ) / 8, ( currHeight + 8 - 1 ) / 8, 1 );

            prevView   = currView;
            prevWidth  = currWidth;
            prevHeight = currHeight;
        }

        computePassEnc.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor;
        cmdBufferDescriptor.label          = "Mip Map Command Buffer";
        wgpu::CommandBuffer mipmapCommands = encoder.Finish( &cmdBufferDescriptor );
        device.GetQueue().Submit( 1, &mipmapCommands );
    }

    wgpu::Buffer downloadTexture( const wgpu::Texture& texture, const wgpu::Device& device, const wgpu::CommandEncoder& encoder, int mipLevel )
    {
        if( texture.GetFormat() != wgpu::TextureFormat::RGBA8Unorm )
        {
            return {};
        }

        int mipWidth  = std::max<int>( 1, texture.GetWidth() / std::pow( 2, mipLevel ) );
        int mipHeight = std::max<int>( 1, texture.GetHeight() / std::pow( 2, mipLevel ) );

        // we need a width thats a multiple of 256
        size_t textureWidthPadded = ( mipWidth + 256 ) / 256 * 256;

        size_t textureSize = textureWidthPadded * mipHeight * 4;

        wgpu::BufferDescriptor layerBufDesc;
        layerBufDesc.mappedAtCreation = false;
        layerBufDesc.size             = textureSize;
        layerBufDesc.usage            = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;

        wgpu::Buffer copyBuffer = device.CreateBuffer( &layerBufDesc );

        wgpu::TexelCopyTextureInfo copyTextureDescritor;
        copyTextureDescritor.texture  = texture;
        copyTextureDescritor.origin   = { 0, 0 };
        copyTextureDescritor.mipLevel = mipLevel;

        wgpu::TexelCopyBufferInfo copyBufferDescriptor;
        copyBufferDescriptor.buffer = copyBuffer;

        // this needs to be multiple of 256
        copyBufferDescriptor.layout.bytesPerRow  = textureWidthPadded * 4;
        copyBufferDescriptor.layout.rowsPerImage = mipHeight;

        wgpu::Extent3D copySize;
        copySize.width  = mipWidth;
        copySize.height = mipHeight;

        encoder.CopyTextureToBuffer( &copyTextureDescritor, &copyBufferDescriptor, &copySize );

        return std::move( copyBuffer );
    }

    wgpu::Device requestDevice( const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor* descriptor )
    {
        struct UserData
        {
            wgpu::Device device;
            bool requestEnded = false;
        };
        UserData userData;

        auto onDeviceRequestEnded = [&userData]( wgpu::RequestDeviceStatus status, wgpu::Device device, const char* message )
        {
            if( status == wgpu::RequestDeviceStatus::Success )
            {
                userData.device = std::move( device );
            }
            else
            {
                SDL_Log( "Could not get WebGPU device: %s", message );
            }
            userData.requestEnded = true;
        };

        adapter.RequestDevice( descriptor, wgpu::CallbackMode::AllowSpontaneous, onDeviceRequestEnded );

        // request device is async on web so hacky solution for now is to sleep
#if defined( SDL_PLATFORM_EMSCRIPTEN )
        while( !userData.requestEnded )
        {
            emscripten_sleep( 50 );
        }
#endif

        return std::move( userData.device );
    }

    wgpu::Adapter requestAdapter( const wgpu::Instance& instance, const wgpu::RequestAdapterOptions* options )
    {
        struct UserData
        {
            wgpu::Adapter adapter;
            bool requestEnded = false;
        };
        UserData userData;

        auto onAdapterRequestEnded = [&userData]( wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, const char* message )
        {
            if( status == wgpu::RequestAdapterStatus::Success )
            {
                userData.adapter = std::move( adapter );
            }
            else
            {
                SDL_Log( "Could not get WebGPU adapter %s", message );
            }
            userData.requestEnded = true;
        };

        instance.RequestAdapter( options, wgpu::CallbackMode::AllowSpontaneous, onAdapterRequestEnded );

        // request adapter is async on web so hacky solution for now is to sleep
#if defined( SDL_PLATFORM_EMSCRIPTEN )
        while( !userData.requestEnded )
        {
            emscripten_sleep( 50 );
        }
#endif

        return std::move( userData.adapter );
    }

} // namespace mc