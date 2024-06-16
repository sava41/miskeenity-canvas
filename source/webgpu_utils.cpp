#include "webgpu_utils.h"

#include "embedded_files.h"

#include <array>

namespace mc
{

    void initMainPipeline( mc::AppContext* app )
    {
        // we have two vertex buffers, our one for verticies and the other for instances
        std::array<wgpu::VertexBufferLayout, 2> vertexBufLayout;

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

        // attributes as defined the in mc::Layer struct
        std::array<wgpu::VertexAttribute, 11> instanceAttr;
        instanceAttr[0].format         = wgpu::VertexFormat::Float32;
        instanceAttr[0].offset         = 0;
        instanceAttr[0].shaderLocation = 2;

        instanceAttr[1].format         = wgpu::VertexFormat::Float32;
        instanceAttr[1].offset         = 1 * sizeof( float );
        instanceAttr[1].shaderLocation = 3;

        instanceAttr[2].format         = wgpu::VertexFormat::Float32;
        instanceAttr[2].offset         = 2 * sizeof( float );
        instanceAttr[2].shaderLocation = 4;

        instanceAttr[3].format         = wgpu::VertexFormat::Float32;
        instanceAttr[3].offset         = 3 * sizeof( float );
        instanceAttr[3].shaderLocation = 5;

        instanceAttr[4].format         = wgpu::VertexFormat::Float32;
        instanceAttr[4].offset         = 4 * sizeof( float );
        instanceAttr[4].shaderLocation = 6;

        instanceAttr[5].format         = wgpu::VertexFormat::Float32;
        instanceAttr[5].offset         = 5 * sizeof( float );
        instanceAttr[5].shaderLocation = 7;

        instanceAttr[6].format         = wgpu::VertexFormat::Uint16x2;
        instanceAttr[6].offset         = 6 * sizeof( float );
        instanceAttr[6].shaderLocation = 8;

        instanceAttr[7].format         = wgpu::VertexFormat::Uint16x2;
        instanceAttr[7].offset         = 6 * sizeof( float ) + 2 * sizeof( uint16_t );
        instanceAttr[7].shaderLocation = 9;

        instanceAttr[8].format         = wgpu::VertexFormat::Uint16x2;
        instanceAttr[8].offset         = 6 * sizeof( float ) + 4 * sizeof( uint16_t );
        instanceAttr[8].shaderLocation = 10;

        instanceAttr[9].format         = wgpu::VertexFormat::Uint8x4;
        instanceAttr[9].offset         = 6 * sizeof( float ) + 6 * sizeof( uint16_t );
        instanceAttr[9].shaderLocation = 11;

        instanceAttr[10].format         = wgpu::VertexFormat::Uint32;
        instanceAttr[10].offset         = 6 * sizeof( float ) + 6 * sizeof( uint16_t ) + 4 * sizeof( uint8_t );
        instanceAttr[10].shaderLocation = 12;

        vertexBufLayout[1].stepMode       = wgpu::VertexStepMode::Instance;
        vertexBufLayout[1].arrayStride    = sizeof( mc::Layer );
        vertexBufLayout[1].attributeCount = static_cast<uint32_t>( instanceAttr.size() );
        vertexBufLayout[1].attributes     = instanceAttr.data();

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
        std::array<wgpu::BindGroupLayoutEntry, 1> globalGroupLayoutEntries;
        globalGroupLayoutEntries[0].binding                 = 0;
        globalGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
        globalGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
        globalGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::Uniform;
        globalGroupLayoutEntries[0].buffer.minBindingSize   = sizeof( mc::Uniforms );

        wgpu::BindGroupLayoutDescriptor globalGroupLayoutDesc;
        globalGroupLayoutDesc.entryCount = static_cast<uint32_t>( globalGroupLayoutEntries.size() );
        globalGroupLayoutDesc.entries    = globalGroupLayoutEntries.data();

        wgpu::BindGroupLayout globalGroupLayout = app->device.CreateBindGroupLayout( &globalGroupLayoutDesc );

        // Create main bind group layout
        std::array<wgpu::BindGroupLayoutEntry, 2> mainGroupLayoutEntries;

        mainGroupLayoutEntries[0].binding      = 0;
        mainGroupLayoutEntries[0].visibility   = wgpu::ShaderStage::Fragment;
        mainGroupLayoutEntries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

        mainGroupLayoutEntries[1].binding               = 1;
        mainGroupLayoutEntries[1].visibility            = wgpu::ShaderStage::Fragment;
        mainGroupLayoutEntries[1].texture.sampleType    = wgpu::TextureSampleType::Float;
        mainGroupLayoutEntries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        wgpu::BindGroupLayoutDescriptor mainGroupLayoutDesc;
        mainGroupLayoutDesc.entryCount = static_cast<uint32_t>( mainGroupLayoutEntries.size() );
        mainGroupLayoutDesc.entries    = mainGroupLayoutEntries.data();

        wgpu::BindGroupLayout mainGroupLayout = app->device.CreateBindGroupLayout( &mainGroupLayoutDesc );

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
        layerBufDesc.usage            = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage;
        app->layerBuf                 = app->device.CreateBuffer( &layerBufDesc );

        wgpu::BufferDescriptor selectionOutputBufDesc;
        selectionOutputBufDesc.mappedAtCreation = false;
        selectionOutputBufDesc.size             = mc::NumLayers * sizeof( mc::Selection );
        selectionOutputBufDesc.usage            = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        app->selectionBuf                       = app->device.CreateBuffer( &selectionOutputBufDesc );

        selectionOutputBufDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
        app->selectionMapBuf         = app->device.CreateBuffer( &selectionOutputBufDesc );

        // Create the bind group for the uniform and shared buffers
        std::array<wgpu::BindGroupEntry, 1> globalGroupEntries;
        globalGroupEntries[0].binding = 0;
        globalGroupEntries[0].buffer  = app->viewParamBuf;
        globalGroupEntries[0].size    = uboBufDesc.size;

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

        std::array<wgpu::BindGroupLayoutEntry, 2> computeGroupLayoutEntries;

        computeGroupLayoutEntries[0].binding                 = 0;
        computeGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Compute;
        computeGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
        computeGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::Storage;
        computeGroupLayoutEntries[0].buffer.minBindingSize   = sizeof( mc::Selection );

        computeGroupLayoutEntries[1].binding                 = 1;
        computeGroupLayoutEntries[1].visibility              = wgpu::ShaderStage::Compute;
        computeGroupLayoutEntries[1].buffer.hasDynamicOffset = false;
        computeGroupLayoutEntries[1].buffer.type             = wgpu::BufferBindingType::ReadOnlyStorage;
        computeGroupLayoutEntries[1].buffer.minBindingSize   = sizeof( mc::Layer );

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

        std::array<wgpu::BindGroupEntry, 2> computeGroupEntries;

        computeGroupEntries[0].binding = 0;
        computeGroupEntries[0].buffer  = app->selectionBuf;
        computeGroupEntries[0].offset  = 0;
        computeGroupEntries[0].size    = selectionOutputBufDesc.size;

        computeGroupEntries[1].binding = 1;
        computeGroupEntries[1].buffer  = app->layerBuf;
        computeGroupEntries[1].offset  = 0;
        computeGroupEntries[1].size    = layerBufDesc.size;

        wgpu::BindGroupDescriptor computeBindGroupDesc;
        computeBindGroupDesc.layout     = computeGroupLayout;
        computeBindGroupDesc.entryCount = static_cast<uint32_t>( computeGroupEntries.size() );
        computeBindGroupDesc.entries    = computeGroupEntries.data();

        app->selectionBindGroup = app->device.CreateBindGroup( &computeBindGroupDesc );
    }

    void initSwapChain( mc::AppContext* app )
    {

        wgpu::SwapChainDescriptor swapChainDesc;
        swapChainDesc.width  = static_cast<uint32_t>( app->width );
        swapChainDesc.height = static_cast<uint32_t>( app->height );

        swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment, swapChainDesc.format = app->colorFormat;
        swapChainDesc.presentMode = wgpu::PresentMode::Fifo;

        app->swapchain = app->device.CreateSwapChain( app->surface, &swapChainDesc );

        app->updateView = true;
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
            UserData& userData = *reinterpret_cast<UserData*>( pUserData );
            if( status == WGPURequestDeviceStatus_Success )
            {
                userData.device = wgpu::Device::Acquire( device );
            }
            else
            {
                SDL_Log( "Could not get WebGPU device: %s", message );
            }
            userData.requestEnded = true;
        };

        adapter.RequestDevice( descriptor, onDeviceRequestEnded, reinterpret_cast<void*>( &userData ) );

        // request device is async on web so hacky solution for now is to sleep
#if defined( SDL_PLATFORM_EMSCRIPTEN )
        emscripten_sleep( 100 );
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
            UserData& userData = *reinterpret_cast<UserData*>( pUserData );
            if( status == WGPURequestAdapterStatus_Success )
            {
                userData.adapter = wgpu::Adapter::Acquire( adapter );
            }
            else
            {
                SDL_Log( "Could not get WebGPU adapter %s", message );
            }
            userData.requestEnded = true;
        };

        instance.RequestAdapter( options, onAdapterRequestEnded, (void*)&userData );

        // request adapter is async on web so hacky solution for now is to sleep
#if defined( SDL_PLATFORM_EMSCRIPTEN )
        emscripten_sleep( 100 );
#endif

        return std::move( userData.adapter );
    }

} // namespace mc