#include "graphics.h"

#include "battery/embed.hpp"

#include <array>
#if defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgpu.h>
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

        wgpu::SupportedLimits supportedLimits;
#if defined( SDL_PLATFORM_EMSCRIPTEN )
        // Error in Chrome: Aborted(TODO: wgpuAdapterGetLimits unimplemented)
        // (as of September 4, 2023), so we hardcode values:
        // These work for 99.95% of clients (source: https://web3dsurvey.com/webgpu)
        supportedLimits.limits.minStorageBufferOffsetAlignment = 256;
        supportedLimits.limits.minUniformBufferOffsetAlignment = 256;
#else
        app->adapter.GetLimits( &supportedLimits );
#endif

        wgpu::RequiredLimits requiredLimits;
        requiredLimits.limits.maxVertexAttributes = 4;
        requiredLimits.limits.maxVertexBuffers    = 2;
        // requiredLimits.limits.maxBufferSize = 150000 * sizeof(WGPUVertexAttributes);
        // requiredLimits.limits.maxVertexBufferArrayStride = sizeof(WGPUVertexAttributes);
        requiredLimits.limits.maxUniformBufferBindingSize     = mc::NumLayers * sizeof( mc::Layer );
        requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
        requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
        requiredLimits.limits.maxInterStageShaderComponents   = 8;
        requiredLimits.limits.maxBindGroups                   = 4;
        requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
        requiredLimits.limits.maxUniformBufferBindingSize     = 16 * 4 * sizeof( float );
        // Allow textures up to 2K
        requiredLimits.limits.maxTextureDimension1D            = 2048;
        requiredLimits.limits.maxTextureDimension2D            = 2048;
        requiredLimits.limits.maxTextureArrayLayers            = 1;
        requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
        requiredLimits.limits.maxSamplersPerShaderStage        = 1;

        wgpu::DeviceDescriptor deviceDesc;
        deviceDesc.label = "Device";
        // deviceDesc.requiredLimits = &requiredLimits;
        deviceDesc.defaultQueue.label = "Main Queue";
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        deviceDesc.uncapturedErrorCallbackInfo.callback = []( WGPUErrorType type, char const* message, void* userData )
        {
            SDL_Log( "Device error type: %d\n", type );
            SDL_Log( "Device error message: %s\n", message );

            mc::AppContext* app = static_cast<mc::AppContext*>( userData );
            app->appQuit        = true;
        };
        deviceDesc.uncapturedErrorCallbackInfo.userdata = app;
#endif
        app->device = mc::requestDevice( app->adapter, &deviceDesc );

#if defined( SDL_PLATFORM_EMSCRIPTEN )
        wgpu::SurfaceCapabilities capabilities;
        app->surface.GetCapabilities( app->adapter, &capabilities );
        app->colorFormat = capabilities.formats[0];
#else
        app->colorFormat = wgpu::TextureFormat::BGRA8Unorm;
#endif
        return true;
    }

    void initMainPipeline( mc::AppContext* app )
    {
        // we have one vertex buffers
        // the vertex buffer is populated in the mesh assember compute shader
        std::array<wgpu::VertexBufferLayout, 1> vertexBufLayout;

        // we have two vertex attributes, uv and 2d position
        std::array<wgpu::VertexAttribute, 2> vertexAttr;
        vertexAttr[0].format         = wgpu::VertexFormat::Float32x2;
        vertexAttr[0].offset         = 0;
        vertexAttr[0].shaderLocation = 0;

        vertexAttr[1].format         = wgpu::VertexFormat::Float32x2;
        vertexAttr[1].offset         = 2 * sizeof( float );
        vertexAttr[1].shaderLocation = 1;

        vertexBufLayout[0].arrayStride    = 4 * sizeof( float );
        vertexBufLayout[0].attributeCount = static_cast<uint32_t>( vertexAttr.size() );
        vertexBufLayout[0].attributes     = vertexAttr.data();

        wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
        shaderCodeDesc.code = b::embed<"./resources/shaders/layers.wgsl">().data();

        wgpu::ShaderModuleDescriptor shaderDesc;
        shaderDesc.nextInChain = &shaderCodeDesc;

        wgpu::ShaderModule shaderModule = app->device.CreateShaderModule( &shaderDesc );

        wgpu::BlendState blendState;
        // Usual alpha blending for the color:
        blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blendState.color.operation = wgpu::BlendOperation::Add;
        // We leave the target alpha untouched:
        blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
        blendState.alpha.dstFactor = wgpu::BlendFactor::One;
        blendState.alpha.operation = wgpu::BlendOperation::Add;

        wgpu::ColorTargetState colorTarget;
        colorTarget.format    = app->colorFormat;
        colorTarget.blend     = &blendState;
        colorTarget.writeMask = wgpu::ColorWriteMask::All;

        wgpu::FragmentState fragmentState;
        fragmentState.module        = shaderModule;
        fragmentState.entryPoint    = "fs_main";
        fragmentState.constantCount = 0;
        fragmentState.targetCount   = 1;
        fragmentState.targets       = &colorTarget;

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

        // Create texture bind group layout for main pipeline
        // we might add more non texture stuff to this pipeline in the future
        wgpu::BindGroupLayout mainGroupLayout = createTextureBindGroupLayout( app->device );

        // Create main pipeline
        std::array<wgpu::BindGroupLayout, 2> mainBindGroupLayouts = { globalGroupLayout, mainGroupLayout };

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

        app->mainPipeline = app->device.CreateRenderPipeline( &renderPipelineDesc );

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
        selectionOutputBufDesc.size             = mc::NumLayers * sizeof( mc::Selection );
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

        // Create layer quad vertex buffer
        const std::vector<float> SquareVertexData{
            -0.5, -0.5, 0.0, 0.0, +0.5, -0.5, 1.0, 0.0, +0.5, +0.5, 1.0, 1.0,

            -0.5, -0.5, 0.0, 0.0, +0.5, +0.5, 1.0, 1.0, -0.5, +0.5, 0.0, 1.0,
        };

        wgpu::BufferDescriptor vertexBufferDesc;
        vertexBufferDesc.mappedAtCreation = true;
        vertexBufferDesc.size             = SquareVertexData.size() * sizeof( float );
        vertexBufferDesc.usage            = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage;
        app->vertexBuf                    = app->device.CreateBuffer( &vertexBufferDesc );
        std::memcpy( app->vertexBuf.GetMappedRange(), SquareVertexData.data(), vertexBufferDesc.size );
        app->vertexBuf.Unmap();

        // Set up compute shader used to compute selection
        {
            wgpu::ShaderModuleWGSLDescriptor selectionShaderCodeDesc;
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

            std::array<wgpu::BindGroupLayout, 2> selectionBindGroupLayouts = { globalGroupLayout, selectionGroupLayout };

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

            wgpu::ShaderModuleWGSLDescriptor meshShaderCodeDesc;
            meshShaderCodeDesc.code = b::embed<"./resources/shaders/mesh.wgsl">().data();

            wgpu::ShaderModuleDescriptor meshShaderModuleDesc;
            meshShaderModuleDesc.nextInChain = &meshShaderCodeDesc;

            wgpu::ShaderModule meshShaderModule = app->device.CreateShaderModule( &meshShaderModuleDesc );

            std::array<wgpu::BindGroupLayoutEntry, 2> meshGroupLayoutEntries;

            meshGroupLayoutEntries[0].binding                 = 0;
            meshGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Compute;
            meshGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
            meshGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::Storage;

            meshGroupLayoutEntries[1].binding                 = 1;
            meshGroupLayoutEntries[1].visibility              = wgpu::ShaderStage::Compute;
            meshGroupLayoutEntries[1].buffer.hasDynamicOffset = false;
            meshGroupLayoutEntries[1].buffer.type             = wgpu::BufferBindingType::Storage;

            wgpu::BindGroupLayoutDescriptor meshBindGroupLayoutDesc;
            meshBindGroupLayoutDesc.entryCount = static_cast<uint32_t>( meshGroupLayoutEntries.size() );
            meshBindGroupLayoutDesc.entries    = meshGroupLayoutEntries.data();

            wgpu::BindGroupLayout meshGroupLayout = app->device.CreateBindGroupLayout( &meshBindGroupLayoutDesc );

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

            // std::array<wgpu::BindGroupEntry, 2> meshGroupEntries;

            // meshGroupEntries[0].binding = 0;
            // meshGroupEntries[0].buffer  = app->meshBuf;
            // meshGroupEntries[0].offset  = 0;
            // meshGroupEntries[0].size    = 0;

            // meshGroupEntries[1].binding = 1;
            // meshGroupEntries[1].buffer  = app->vertexBuf;
            // meshGroupEntries[1].offset  = 0;
            // meshGroupEntries[1].size    = vertexBufferDesc.size;

            // wgpu::BindGroupDescriptor meshBindGroupDesc;
            // meshBindGroupDesc.layout     = meshGroupLayout;
            // meshBindGroupDesc.entryCount = static_cast<uint32_t>( meshGroupEntries.size() );
            // meshBindGroupDesc.entries    = meshGroupEntries.data();

            // app->meshBindGroup = app->device.CreateBindGroup( &meshBindGroupDesc );
        }
    }

    void configureSurface( mc::AppContext* app )
    {
        wgpu::SurfaceConfiguration config;
        config.device = app->device;
        config.format = app->colorFormat;
        config.width  = app->width;
        config.height = app->height;
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

        if( app->vertexBuf == nullptr )
        {
            // wgpu::BufferDescriptor vertexBufDesc;
            // vertexBufDesc.mappedAtCreation = false;
            // vertexBufDesc.size             = app->meshManager.length();
            // vertexBufDesc.usage            = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
            // app->vertexBuf                 = app->device.CreateBuffer( &vertexBufDesc );
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
        ;

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
        groupLayoutEntries[0].visibility   = wgpu::ShaderStage::Fragment;
        groupLayoutEntries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

        groupLayoutEntries[1].binding               = 1;
        groupLayoutEntries[1].visibility            = wgpu::ShaderStage::Fragment;
        groupLayoutEntries[1].texture.sampleType    = wgpu::TextureSampleType::Float;
        groupLayoutEntries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        wgpu::BindGroupLayoutDescriptor groupLayoutDesc;
        groupLayoutDesc.entryCount = static_cast<uint32_t>( groupLayoutEntries.size() );
        groupLayoutDesc.entries    = groupLayoutEntries.data();

        return device.CreateBindGroupLayout( &groupLayoutDesc );
    }

    void uploadTexture( const wgpu::Queue& queue, const wgpu::Texture& texture, void* data, int width, int height, int channels )
    {
        wgpu::ImageCopyTexture imageCopyTexture;
        imageCopyTexture.texture  = texture;
        imageCopyTexture.mipLevel = 0;
        imageCopyTexture.origin   = { 0, 0, 0 };
        imageCopyTexture.aspect   = wgpu::TextureAspect::All;

        wgpu::TextureDataLayout textureDataLayout;
        textureDataLayout.offset       = 0;
        textureDataLayout.bytesPerRow  = width * 4;
        textureDataLayout.rowsPerImage = height;

        wgpu::Extent3D writeSize;
        writeSize.width              = width;
        writeSize.height             = height;
        writeSize.depthOrArrayLayers = 1;

        queue.WriteTexture( &imageCopyTexture, data, width * height * channels * sizeof( uint8_t ), &textureDataLayout, &writeSize );
    }

    wgpu::Device requestDevice( const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor* descriptor )
    {
        struct UserData
        {
            wgpu::Device device;
            bool requestEnded = false;
        };
        UserData userData;

        auto onDeviceRequestEnded = []( WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData )
        {
            UserData* userData = reinterpret_cast<UserData*>( pUserData );
            if( status == WGPURequestDeviceStatus_Success )
            {
                userData->device = wgpu::Device::Acquire( device );
            }
            else
            {
                SDL_Log( "Could not get WebGPU device: %s", message );
            }
            userData->requestEnded = true;
        };

        adapter.RequestDevice( descriptor, onDeviceRequestEnded, reinterpret_cast<void*>( &userData ) );

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

        auto onAdapterRequestEnded = []( WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData )
        {
            UserData* userData = reinterpret_cast<UserData*>( pUserData );
            if( status == WGPURequestAdapterStatus_Success )
            {
                userData->adapter = wgpu::Adapter::Acquire( adapter );
            }
            else
            {
                SDL_Log( "Could not get WebGPU adapter %s", message );
            }
            userData->requestEnded = true;
        };

        instance.RequestAdapter( options, onAdapterRequestEnded, (void*)&userData );

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