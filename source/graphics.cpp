#include "graphics.h"

#include "embedded_files.h"

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
        // we have two vertex buffers, our one for verticies and the other for instances
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
        shaderCodeDesc.code = reinterpret_cast<const char*>( layers_wgsl );

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

        // Create main bind group layout
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
        globalGroupEntries[0].size    = uboBufDesc.size;

        globalGroupEntries[1].binding = 1;
        globalGroupEntries[1].buffer  = app->layerBuf;
        globalGroupEntries[1].size    = layerBufDesc.size;

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

        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.mappedAtCreation = true;
        bufferDesc.size             = SquareVertexData.size() * sizeof( float );
        bufferDesc.usage            = wgpu::BufferUsage::Vertex;
        app->vertexBuf              = app->device.CreateBuffer( &bufferDesc );
        std::memcpy( app->vertexBuf.GetMappedRange(), SquareVertexData.data(), bufferDesc.size );
        app->vertexBuf.Unmap();

        // Set up compute shader used to compute selection
        wgpu::ShaderModuleWGSLDescriptor computeShaderCodeDesc;
        computeShaderCodeDesc.code = reinterpret_cast<const char*>( selection_wgsl );

        wgpu::ShaderModuleDescriptor computeShaderModuleDesc;
        computeShaderModuleDesc.nextInChain = &computeShaderCodeDesc;

        wgpu::ShaderModule computeShaderModule = app->device.CreateShaderModule( &computeShaderModuleDesc );

        std::array<wgpu::BindGroupLayoutEntry, 1> computeGroupLayoutEntries;

        computeGroupLayoutEntries[0].binding                 = 0;
        computeGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Compute;
        computeGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
        computeGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::Storage;
        computeGroupLayoutEntries[0].buffer.minBindingSize   = sizeof( mc::Selection );

        wgpu::BindGroupLayoutDescriptor computeBindGroupLayoutDesc;
        computeBindGroupLayoutDesc.entryCount = static_cast<uint32_t>( computeGroupLayoutEntries.size() );
        computeBindGroupLayoutDesc.entries    = computeGroupLayoutEntries.data();

        wgpu::BindGroupLayout computeGroupLayout = app->device.CreateBindGroupLayout( &computeBindGroupLayoutDesc );

        std::array<wgpu::BindGroupLayout, 2> computeBindGroupLayouts = { globalGroupLayout, computeGroupLayout };

        wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc;
        computePipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>( computeBindGroupLayouts.size() );
        computePipelineLayoutDesc.bindGroupLayouts     = computeBindGroupLayouts.data();

        wgpu::PipelineLayout computePipelineLayout = app->device.CreatePipelineLayout( &computePipelineLayoutDesc );

        wgpu::ComputePipelineDescriptor computePipelineDesc;
        computePipelineDesc.label              = "Canvas Selection";
        computePipelineDesc.layout             = computePipelineLayout;
        computePipelineDesc.compute.module     = computeShaderModule;
        computePipelineDesc.compute.entryPoint = "cs_main";

        app->selectionPipeline = app->device.CreateComputePipeline( &computePipelineDesc );

        std::array<wgpu::BindGroupEntry, 1> computeGroupEntries;

        computeGroupEntries[0].binding = 0;
        computeGroupEntries[0].buffer  = app->selectionBuf;
        computeGroupEntries[0].offset  = 0;
        computeGroupEntries[0].size    = selectionOutputBufDesc.size;

        wgpu::BindGroupDescriptor computeBindGroupDesc;
        computeBindGroupDesc.layout     = computeGroupLayout;
        computeBindGroupDesc.entryCount = static_cast<uint32_t>( computeGroupEntries.size() );
        computeBindGroupDesc.entries    = computeGroupEntries.data();

        app->selectionBindGroup = app->device.CreateBindGroup( &computeBindGroupDesc );
    }

    void configureSurface( mc::AppContext* app )
    {
        wgpu::SurfaceConfiguration config;
        config.device = app->device;
        config.format = app->colorFormat;
        config.width  = app->width;
        config.height = app->height;
        app->surface.Configure( &config );

        app->updateView = true;
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