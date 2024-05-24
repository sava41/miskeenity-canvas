#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <array>
#include <glm/glm.hpp>
#include <imgui.h>
#include <string>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#if defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include "app.h"
#include "embedded_files.h"
#include "layers.h"
#include "ui.h"
#include "webgpu_surface.h"

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

    // we have 9 instance attributes as defined the in mc::Layer struct
    // we combine some of them to form 7 vertex attributes
    std::array<wgpu::VertexAttribute, 7> instanceAttr;
    instanceAttr[0].format         = wgpu::VertexFormat::Float32x2;
    instanceAttr[0].offset         = 0;
    instanceAttr[0].shaderLocation = 2;

    instanceAttr[1].format         = wgpu::VertexFormat::Float32x2;
    instanceAttr[1].offset         = 2 * sizeof( float );
    instanceAttr[1].shaderLocation = 3;

    instanceAttr[2].format         = wgpu::VertexFormat::Float32x2;
    instanceAttr[2].offset         = 4 * sizeof( float );
    instanceAttr[2].shaderLocation = 4;

    instanceAttr[3].format         = wgpu::VertexFormat::Uint16x2;
    instanceAttr[3].offset         = 6 * sizeof( float );
    instanceAttr[3].shaderLocation = 5;

    instanceAttr[4].format         = wgpu::VertexFormat::Uint16x2;
    instanceAttr[4].offset         = 6 * sizeof( float ) + 2 * sizeof( uint16_t );
    instanceAttr[4].shaderLocation = 6;

    instanceAttr[5].format         = wgpu::VertexFormat::Uint16x2;
    instanceAttr[5].offset         = 6 * sizeof( float ) + 4 * sizeof( uint16_t );
    instanceAttr[5].shaderLocation = 7;

    instanceAttr[6].format         = wgpu::VertexFormat::Uint8x4;
    instanceAttr[6].offset         = 6 * sizeof( float ) + 6 * sizeof( uint16_t );
    instanceAttr[6].shaderLocation = 8;

    vertexBufLayout[1].stepMode       = wgpu::VertexStepMode::Instance;
    vertexBufLayout[1].arrayStride    = sizeof( mc::Layer );
    vertexBufLayout[1].attributeCount = static_cast<uint32_t>( instanceAttr.size() );
    vertexBufLayout[1].attributes     = instanceAttr.data();

    // Create main uber shader
    const std::string shaderSource = R"(

        struct VertexInput {
            @builtin(vertex_index) vertexId: u32,
            @builtin(instance_index) instanceId: u32,
            @location(0) position: vec2<f32>,
            @location(1) uv: vec2<f32>,
        };

        struct InstanceInput {
            @location(2) offset: vec2<f32>,
            @location(3) basis_a: vec2<f32>,
            @location(4) basis_b: vec2<f32>,
            @location(5) uv_top: vec2<u32>,
            @location(6) uv_bot: vec2<u32>,
            @location(7) image_mask_ids: vec2<u32>,
            @location(8) color_type: vec4<u32>,
        };

        struct VertexOutput {
            @builtin(position) position: vec4<f32>,
            @location(0) uv: vec2<f32>,
            @location(1) color: vec4<f32>,
            @location(2) @interpolate(flat) inst: u32,
        };

        struct Uniforms {
            proj: mat4x4<f32>,
            canvasPos: vec2<f32>,
            mousePos: vec2<f32>,
            mouseSelectPos: vec2<f32>,
            windowSize: vec2<u32>,
            scale: f32,
        };

        @group(0) @binding(0)
        var<uniform> uniforms: Uniforms;
        @group(0) @binding(1)
        var<storage,read_write> selectionFlags: array<u32>;

        @vertex
        fn vs_main(vert: VertexInput, inst: InstanceInput) -> VertexOutput {
            var out: VertexOutput;

            let model = mat4x4<f32>(inst.basis_a.x, inst.basis_b.x, 0.0, inst.offset.x,
                                    inst.basis_a.y, inst.basis_b.y, 0.0, inst.offset.y,
                                    0.0,            0.0,            1.0, 0.0,
                                    0.0,            0.0,            0.0, 1.0);

            out.position = vec4<f32>(vert.position, 0.0, 1.0) * model * uniforms.proj ;
            out.uv = vert.uv;
            out.inst = vert.instanceId;

            out.color = vec4<f32>(f32(inst.color_type.r) / 255.0, f32(inst.color_type.g) / 255.0, f32(inst.color_type.b) / 255.0, 1.0);

            return out;
        }

        @fragment
        fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
            return in.color + (vec4<f32>(in.uv, 1.0, 1.0) * 0.3 - 0.15) * f32(selectionFlags[in.inst] == 1);
        }
    )";

    wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
    shaderCodeDesc.code = shaderSource.c_str();

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
    globalGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Compute;
    globalGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
    globalGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::Uniform;
    globalGroupLayoutEntries[0].buffer.minBindingSize   = sizeof( mc::Uniforms );

    globalGroupLayoutEntries[1].binding                 = 1;
    globalGroupLayoutEntries[1].visibility              = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
    globalGroupLayoutEntries[1].buffer.hasDynamicOffset = false;
    globalGroupLayoutEntries[1].buffer.type             = wgpu::BufferBindingType::Storage;

    wgpu::BindGroupLayoutDescriptor globalGroupLayoutDesc;
    globalGroupLayoutDesc.entryCount = static_cast<uint32_t>( globalGroupLayoutEntries.size() );
    globalGroupLayoutDesc.entries    = globalGroupLayoutEntries.data();

    wgpu::BindGroupLayout globalGroupLayout = app->device.CreateBindGroupLayout( &globalGroupLayoutDesc );

    // Create main pipeline
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts     = &globalGroupLayout;

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
    selectionOutputBufDesc.size             = sizeof( float ) * mc::NumLayers;
    selectionOutputBufDesc.usage            = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    app->selectionBuf                       = app->device.CreateBuffer( &selectionOutputBufDesc );

    selectionOutputBufDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    app->selectionMapBuf         = app->device.CreateBuffer( &selectionOutputBufDesc );

    // Create the bind group for the uniform and shared buffers
    std::array<wgpu::BindGroupEntry, 2> globalGroupEntries;
    globalGroupEntries[0].binding = 0;
    globalGroupEntries[0].buffer  = app->viewParamBuf;
    globalGroupEntries[0].size    = uboBufDesc.size;

    globalGroupEntries[1].binding = 1;
    globalGroupEntries[1].buffer  = app->selectionBuf;
    globalGroupEntries[1].offset  = 0;
    globalGroupEntries[1].size    = selectionOutputBufDesc.size;

    wgpu::BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout     = globalGroupLayout;
    bindGroupDesc.entryCount = static_cast<uint32_t>( globalGroupEntries.size() );
    bindGroupDesc.entries    = globalGroupEntries.data();

    app->bindGroup = app->device.CreateBindGroup( &bindGroupDesc );

    // Create layer quad vertex buffer
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.mappedAtCreation = true;
    bufferDesc.size             = mc::VertexData.size() * sizeof( float );
    bufferDesc.usage            = wgpu::BufferUsage::Vertex;
    app->vertexBuf              = app->device.CreateBuffer( &bufferDesc );
    std::memcpy( app->vertexBuf.GetMappedRange(), mc::VertexData.data(), bufferDesc.size );
    app->vertexBuf.Unmap();

    // Set up compute shader used to compute selection
    const std::string computeShaderSource = R"(
        struct Uniforms {
            proj: mat4x4<f32>,
            canvasPos: vec2<f32>,
            mousePos: vec2<f32>,
            mouseSelectPos: vec2<f32>,
            windowSize: vec2<u32>,
            scale: f32,
        };

        struct InstanceInput {
            offset: vec2<f32>,
            basis_a: vec2<f32>,
            basis_b: vec2<f32>,
            uv_top: u32,
            uv_bot: u32,
            image_mask_ids: u32,
            color_type: u32,
        };

        @group(0) @binding(0)
        var<uniform> uniforms: Uniforms;
        @group(0) @binding(1)
        var<storage,read_write> outBuffer: array<u32>;
        
        @group(1) @binding(0)
        var<storage, read> instanceBuffer: array<InstanceInput>;

        fn barycentric(v1: vec3<f32>, v2: vec3<f32>, v3: vec3<f32>, p: vec2<f32>) -> vec3<f32> {
            let u = cross(
                vec3<f32>(v3.x - v1.x, v2.x - v1.x, v1.x - p.x), 
                vec3<f32>(v3.y - v1.y, v2.y - v1.y, v1.y - p.y)
            );

            if (abs(u.z) < 1.0) {
                return vec3<f32>(-1.0, 1.0, 1.0);
            }

            return vec3<f32>(1.0 - (u.x+u.y)/u.z, u.y/u.z, u.x/u.z); 
        }

        @compute @workgroup_size(256, 1)
        fn cs_main(@builtin(global_invocation_id) id_global : vec3<u32>, @builtin(local_invocation_id) id_local : vec3<u32>) {
            let layer = u32(id_global.x);
            
            let verts = array<vec4<f32>, 4>(
            vec4<f32>( -0.5,  -0.5, 0.0, 1.0),
            vec4<f32>(  0.5,  -0.5, 0.0, 1.0),
            vec4<f32>(  0.5,   0.5, 0.0, 1.0),
            vec4<f32>( -0.5,   0.5, 0.0, 1.0));

            let minX = min(uniforms.mousePos.x, uniforms.mouseSelectPos.x);
            let minY = min(uniforms.mousePos.y, uniforms.mouseSelectPos.y);
            let maxX = max(uniforms.mousePos.x, uniforms.mouseSelectPos.x);
            let maxY = max(uniforms.mousePos.y, uniforms.mouseSelectPos.y);

            let model = mat4x4<f32>(instanceBuffer[layer].basis_a.x,
                                    instanceBuffer[layer].basis_b.x, 0.0,
                                    instanceBuffer[layer].offset.x,
                                    instanceBuffer[layer].basis_a.y,
                                    instanceBuffer[layer].basis_b.y, 0.0,
                                    instanceBuffer[layer].offset.y,
                                    0.0, 0.0, 1.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0);

            // We have 3 possible scenarios:
            // Ractangle is fully inside selection box
            // Rectangle is fully outside selection box
            // Rectangle is partially inside selection box in which case we need to rasterize

            var flags = u32(0);

            for (var i: u32 = 0; i < 4; i = i + 1u) {
                let pos = verts[i] * model;
                
                if(minX < pos.x && pos.x < maxX && minY < pos.y && pos.y < maxY) {
                    flags = flags | 1;
                } else {
                    flags = flags | 2;
                }
            }

            outBuffer[layer] = flags;
        }

    )";

    wgpu::ShaderModuleWGSLDescriptor computeShaderCodeDesc;
    computeShaderCodeDesc.code = computeShaderSource.c_str();

    wgpu::ShaderModuleDescriptor computeShaderModuleDesc;
    computeShaderModuleDesc.nextInChain = &computeShaderCodeDesc;

    wgpu::ShaderModule computeShaderModule = app->device.CreateShaderModule( &computeShaderModuleDesc );

    std::array<wgpu::BindGroupLayoutEntry, 1> computeGroupLayoutEntries;

    computeGroupLayoutEntries[0].binding                 = 0;
    computeGroupLayoutEntries[0].visibility              = wgpu::ShaderStage::Compute;
    computeGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
    computeGroupLayoutEntries[0].buffer.type             = wgpu::BufferBindingType::ReadOnlyStorage;
    computeGroupLayoutEntries[0].buffer.minBindingSize   = sizeof( mc::Layer );

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
    computeGroupEntries[0].buffer  = app->layerBuf;
    computeGroupEntries[0].offset  = 0;
    computeGroupEntries[0].size    = layerBufDesc.size;

    wgpu::BindGroupDescriptor computeBindGroupDesc;
    computeBindGroupDesc.layout     = computeGroupLayout;
    computeBindGroupDesc.entryCount = static_cast<uint32_t>( computeGroupEntries.size() );
    computeBindGroupDesc.entries    = computeGroupEntries.data();

    app->selectionBindGroup = app->device.CreateBindGroup( &computeBindGroupDesc );
}

bool initSwapChain( mc::AppContext* app )
{
    SDL_GetWindowSize( app->window, &app->width, &app->height );
    SDL_GetWindowSizeInPixels( app->window, &app->bbwidth, &app->bbheight );

    wgpu::SwapChainDescriptor swapChainDesc;
    swapChainDesc.width  = static_cast<uint32_t>( app->width );
    swapChainDesc.height = static_cast<uint32_t>( app->height );

    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment, swapChainDesc.format = app->colorFormat;
    swapChainDesc.presentMode = wgpu::PresentMode::Fifo;

    app->swapchain = app->device.CreateSwapChain( app->surface, &swapChainDesc );

    app->updateView = true;

    return true;
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

int SDL_Fail()
{
    SDL_LogError( SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError() );
    return -1;
}

int SDL_AppInit( void** appstate, int argc, char* argv[] )
{
    mc::AppContext* app = new mc::AppContext;

    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMEPAD ) )
    {
        return SDL_Fail();
    }

    SDL_SetHint( SDL_HINT_IME_SHOW_UI, "1" );

    app->window = SDL_CreateWindow( "Miskeenity Canvas", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED );
    if( !app->window )
    {
        return SDL_Fail();
    }

    app->instance = wgpu::CreateInstance();
    if( !app->instance )
    {
        return SDL_Fail();
    }

    app->surface = SDL_GetWGPUSurface( app->instance, app->window );
    if( !app->surface.Get() )
    {
        return SDL_Fail();
    }

    wgpu::RequestAdapterOptions adapterOpts;
    adapterOpts.compatibleSurface = app->surface;

    app->adapter = requestAdapter( app->instance, &adapterOpts );
    if( !app->adapter )
    {
        return SDL_Fail();
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
    app->device                   = requestDevice( app->adapter, &deviceDesc );

#if defined( SDL_PLATFORM_EMSCRIPTEN )
    app->colorFormat = app->surface.GetPreferredFormat( app->adapter );
#else()
    app->colorFormat = wgpu::TextureFormat::BGRA8Unorm;
#endif()

    app->device.SetUncapturedErrorCallback(
        []( WGPUErrorType type, char const* message, void* userData )
        {
            SDL_Log( "Device error type: %d\n", type );
            SDL_Log( "Device error message: %s\n", message );

            mc::AppContext* app = static_cast<mc::AppContext*>( userData );
            app->appQuit        = true;
        },
        app );

    initSwapChain( app );

    mc::initUI( app );

    initMainPipeline( app );

    // print some information about the window
    SDL_ShowWindow( app->window );
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize( app->window, &width, &height );
        SDL_GetWindowSizeInPixels( app->window, &bbwidth, &bbheight );
        SDL_Log( "Window size: %ix%i", width, height );
        SDL_Log( "Backbuffer size: %ix%i", bbwidth, bbheight );
        if( width != bbwidth )
        {
            SDL_Log( "This is a highdpi environment." );
        }
    }

    app->updateView = true;

    *appstate = app;

    SDL_Log( "Application started successfully!" );

    return 0;
}

int SDL_AppEvent( void* appstate, const SDL_Event* event )
{
    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );
    ImGuiIO& io         = ImGui::GetIO();

    switch( event->type )
    {
    case SDL_EVENT_QUIT:
        app->appQuit = true;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        app->resetSwapchain = true;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        io.AddMouseButtonEvent( 0, true );

        if( !io.WantCaptureMouse )
        {
            app->mouseDragStart = app->mouseWindowPos;
            app->mouseDown      = true;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        io.AddMouseButtonEvent( 0, false );

        // we want to place a sprite on click
        if( app->mouseWindowPos == app->mouseDragStart )
        {
            app->addLayer = true;
        }
        app->mouseDown = false;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        io.AddMousePosEvent( event->motion.x, event->motion.y );
        app->mouseWindowPos = glm::vec2( event->motion.x, event->motion.y );
        app->mouseDelta += glm::vec2( event->motion.xrel, event->motion.yrel );

        if( app->mouseDown && !io.WantCaptureMouse )
        {
            app->updateView = true;
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        io.AddMouseWheelEvent( event->wheel.x, event->wheel.y );
        app->scrollDelta += glm::vec2( event->wheel.x, event->wheel.y );

        if( !io.WantCaptureMouse )
        {
            app->updateView = true;
        }
        break;
    default:
        break;
    }

    return 0;
}

int SDL_AppIterate( void* appstate )
{
    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );

    if( app->resetSwapchain )
    {
        if( !initSwapChain( app ) )
        {
            return false;
        }

        app->resetSwapchain = false;
    }

    // Update canvas offset
    if( app->mouseDelta.length() > 0.0 && app->mouseDown && app->updateView && app->state == mc::State::Pan )
    {
        app->viewParams.canvasPos += app->mouseDelta;
    }

    // Update mouse position in canvas coordinate space
    app->viewParams.mousePos = ( app->mouseWindowPos - app->viewParams.canvasPos ) / app->viewParams.scale;

    // Update zoom level
    // For zoom, we want to center it around the mouse position
    if( app->scrollDelta.y != 0.0 && app->updateView )
    {
        float newScale        = std::max<float>( 1.0, app->scrollDelta.y * mc::ZoomScaleFactor + app->viewParams.scale );
        float deltaScale      = newScale - app->viewParams.scale;
        app->viewParams.scale = newScale;
        app->viewParams.canvasPos -= app->viewParams.mousePos * deltaScale;
    }

    if( app->updateView )
    {
        app->viewParams.width  = app->width;
        app->viewParams.height = app->height;

        float l = -app->viewParams.canvasPos.x * 1.0 / app->viewParams.scale;
        float r = ( app->width - app->viewParams.canvasPos.x ) * 1.0 / app->viewParams.scale;
        float t = -app->viewParams.canvasPos.y * 1.0 / app->viewParams.scale;
        float b = ( app->height - app->viewParams.canvasPos.y ) * 1.0 / app->viewParams.scale;

        app->viewParams.proj = glm::mat4( 2.0 / ( r - l ), 0.0, 0.0, ( r + l ) / ( l - r ), 0.0, 2.0 / ( t - b ), 0.0, ( t + b ) / ( b - t ), 0.0, 0.0, 0.5,
                                          0.5, 0.0, 0.0, 0.0, 1.0 );
    }

    if( app->state == mc::State::Cursor && app->mouseDown && app->mouseDragStart != app->mouseWindowPos )
    {
        app->selectionRequested = true;

        app->viewParams.mouseSelectPos = ( app->mouseDragStart - app->viewParams.canvasPos ) / app->viewParams.scale;
    }
    app->device.GetQueue().WriteBuffer( app->viewParamBuf, 0, &app->viewParams, sizeof( mc::Uniforms ) );

    if( app->addLayer && app->state == mc::State::Cursor )
    {
        // temporary generate random color
        const uint32_t a = 1664525;
        const uint32_t c = 1013904223;

        const uint32_t color = a * app->layers.length() + c;
        const uint8_t red    = static_cast<uint8_t>( ( color >> 16 ) & 0xFF );
        const uint8_t green  = static_cast<uint8_t>( ( color >> 8 ) & 0xFF );
        const uint8_t blue   = static_cast<uint8_t>( color & 0xFF );

        app->layers.add( { app->viewParams.mousePos, glm::vec2( 100, 0 ), glm::vec2( 0, 100 ), glm::u16vec2( 0 ), glm::u16vec2( 0 ), 0, 0,
                           glm::u8vec3( red, green, blue ), 0 } );

        app->layersModified = true;
    }

    if( app->layersModified )
    {
        app->device.GetQueue().WriteBuffer( app->layerBuf, 0, app->layers.data(), app->layers.length() * sizeof( mc::Layer ) );
    }

    wgpu::TextureView nextTexture = app->swapchain.GetCurrentTextureView();
    // Getting the texture may fail, in particular if the window has been resized
    // and thus the target surface changed.
    if( !nextTexture )
    {
        SDL_Log( "Cannot acquire next swap chain texture" );
        return false;
    }

    wgpu::CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Casper";

    wgpu::CommandEncoder encoder = app->device.CreateCommandEncoder( &commandEncoderDesc );

    wgpu::RenderPassColorAttachment renderPassColorAttachment;
    renderPassColorAttachment.view = nextTexture, renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear,
    renderPassColorAttachment.storeOp = wgpu::StoreOp::Store, renderPassColorAttachment.clearValue = wgpu::Color{ 0.949f, 0.929f, 0.898f, 1.0f };

    wgpu::RenderPassDescriptor renderPassDesc;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments     = &renderPassColorAttachment;

    wgpu::RenderPassEncoder renderPassEnc = encoder.BeginRenderPass( &renderPassDesc );

    if( app->layers.length() > 0 )
    {
        renderPassEnc.SetPipeline( app->mainPipeline );
        renderPassEnc.SetVertexBuffer( 0, app->vertexBuf );
        renderPassEnc.SetVertexBuffer( 1, app->layerBuf );
        renderPassEnc.SetBindGroup( 0, app->bindGroup );
        renderPassEnc.Draw( 6, app->layers.length(), 0, 0 );
    }

    mc::drawUI( app, renderPassEnc );

    renderPassEnc.End();

    // We only want to recompute the selection array if a selection is requested and any
    // previously requested computations have completed aka selection ready
    if( app->selectionRequested && app->selectionReady && app->layers.length() > 0 )
    {
        encoder.ClearBuffer( app->selectionBuf, 0, app->layers.length() * sizeof( uint32_t ) );
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline( app->selectionPipeline );
        // figure out which things to bind;
        computePass.SetBindGroup( 0, app->bindGroup );
        computePass.SetBindGroup( 1, app->selectionBindGroup );

        computePass.DispatchWorkgroups( ( app->layers.length() + 256 - 1 ) / 256, 1, 1 );
        computePass.End();

        app->selectionMapBuf.Unmap();
        app->selectionFlags = nullptr;

        encoder.CopyBufferToBuffer( app->selectionBuf, 0, app->selectionMapBuf, 0, sizeof( float ) * mc::NumLayers );
    }

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label   = "Melchior";
    wgpu::CommandBuffer command = encoder.Finish( &cmdBufferDescriptor );

    app->device.GetQueue().Submit( 1, &command );

    if( app->selectionRequested && app->selectionReady )
    {
        app->selectionReady              = false;
        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userdata )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                mc::AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );
                app->selectionFlags = reinterpret_cast<uint32_t*>(
                    const_cast<void*>( ( app->selectionMapBuf.GetConstMappedRange( 0, sizeof( float ) * app->layers.length() ) ) ) );
                app->selectionReady = true;
            }
        };
        app->selectionMapBuf.MapAsync( wgpu::MapMode::Read, 0, sizeof( float ) * mc::NumLayers, callback, app );
    }

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->swapchain.Present();
    app->device.Tick();
#endif

    // Reset
    app->mouseDelta         = glm::vec2( 0.0 );
    app->scrollDelta        = glm::vec2( 0.0 );
    app->updateView         = false;
    app->layersModified     = false;
    app->addLayer           = false;
    app->selectionRequested = false;

    return app->appQuit;
}

void SDL_AppQuit( void* appstate )
{


    mc::shutdownUI();

    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );
    if( app )
    {
        SDL_DestroyWindow( app->window );
        delete app;
    }

    SDL_Quit();
    SDL_Log( "Application quit successfully!" );
}
