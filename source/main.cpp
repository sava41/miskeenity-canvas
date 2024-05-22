#include <array>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include "embedded_files.h"
#include "layers.h"
#include "lucide.h"
#include "webgpu_surface.h"

constexpr float ZoomScaleFactor = 0.5;
constexpr size_t NumLayers = 2048;
constexpr size_t WorkGroupSize = 16;

enum State { Cursor, Pan, Paint, Text, Other };

#pragma pack(push)
struct Uniforms {
    glm::mat4 proj;
    glm::vec2 mousePos = glm::vec2(0.0);
    glm::vec2 canvasPos = glm::vec2(0.0);
    uint32_t width;
    uint32_t height;
    float scale = 1.0;
    float _pad[9];
};
#pragma pack(pop)
// Have the compiler check byte alignment
// Total size must be a multiple of the alignment size of its largest field
static_assert(sizeof(Uniforms) % sizeof(glm::mat4) == 0);

struct AppContext {
    SDL_Window *window;

    int width;
    int height;
    int bbwidth;
    int bbheight;

    wgpu::Instance instance;
    wgpu::Surface surface;
    wgpu::Device device;
    wgpu::Adapter adapter;

    wgpu::SwapChain swapchain;
    wgpu::TextureFormat colorFormat;

    wgpu::RenderPipeline mainPipeline;
    wgpu::ComputePipeline selectionPipeline;
    wgpu::Buffer vertexBuf;
    wgpu::Buffer layerBuf;
    wgpu::Buffer viewParamBuf;
    wgpu::Buffer selectionBuf;
    wgpu::Buffer selectionMapBuf;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroup selectionBindGroup;

    Uniforms viewParams;

    State state = Cursor;
    bool updateView = false;
    bool selectionRequested = false;
    bool selectionReady = true;
    bool layersModified = true;
    bool addLayer = false;
    bool appQuit = false;
    bool resetSwapchain = false;

    uint32_t *selectionFlags = nullptr;

    // Input variables
    glm::vec2 mouseWindowPos = glm::vec2(0.0);
    glm::vec2 mouseDragStart = glm::vec2(0.0);
    glm::vec2 mouseDelta = glm::vec2(0.0);
    glm::vec2 scrollDelta = glm::vec2(0.0);
    bool mouseDown = false;

    mc::Layers layers = mc::Layers(NumLayers);
};

void initMainPipeline(AppContext *app)
{
    // we have two vertex buffers, our one for verticies and the other for instances
    std::array<wgpu::VertexBufferLayout, 2> vertexBufLayout;

    // we have two vertex attributes, uv and 2d position
    std::array<wgpu::VertexAttribute, 2> vertexAttr;
    vertexAttr[0].format = wgpu::VertexFormat::Float32x2;
    vertexAttr[0].offset = 0;
    vertexAttr[0].shaderLocation = 0;

    vertexAttr[1].format = wgpu::VertexFormat::Float32x2;
    vertexAttr[1].offset = 2 * sizeof(float);
    vertexAttr[1].shaderLocation = 1;

    vertexBufLayout[0].arrayStride = 4 * sizeof(float);
    vertexBufLayout[0].attributeCount = static_cast<uint32_t>(vertexAttr.size());
    vertexBufLayout[0].attributes = vertexAttr.data();

    // we have 9 instance attributes as defined the in mc::Layer struct
    // we combine some of them to form 7 vertex attributes
    std::array<wgpu::VertexAttribute, 7> instanceAttr;
    instanceAttr[0].format = wgpu::VertexFormat::Float32x2;
    instanceAttr[0].offset = 0;
    instanceAttr[0].shaderLocation = 2;

    instanceAttr[1].format = wgpu::VertexFormat::Float32x2;
    instanceAttr[1].offset = 2 * sizeof(float);
    instanceAttr[1].shaderLocation = 3;

    instanceAttr[2].format = wgpu::VertexFormat::Float32x2;
    instanceAttr[2].offset = 4 * sizeof(float);
    instanceAttr[2].shaderLocation = 4;

    instanceAttr[3].format = wgpu::VertexFormat::Uint16x2;
    instanceAttr[3].offset = 6 * sizeof(float);
    instanceAttr[3].shaderLocation = 5;

    instanceAttr[4].format = wgpu::VertexFormat::Uint16x2;
    instanceAttr[4].offset = 6 * sizeof(float) + 2 * sizeof(uint16_t);
    instanceAttr[4].shaderLocation = 6;

    instanceAttr[5].format = wgpu::VertexFormat::Uint16x2;
    instanceAttr[5].offset = 6 * sizeof(float) + 4 * sizeof(uint16_t);
    instanceAttr[5].shaderLocation = 7;

    instanceAttr[6].format = wgpu::VertexFormat::Uint8x4;
    instanceAttr[6].offset = 6 * sizeof(float) + 6 * sizeof(uint16_t);
    instanceAttr[6].shaderLocation = 8;

    vertexBufLayout[1].stepMode = wgpu::VertexStepMode::Instance;
    vertexBufLayout[1].arrayStride = sizeof(mc::Layer);
    vertexBufLayout[1].attributeCount = static_cast<uint32_t>(instanceAttr.size());
    vertexBufLayout[1].attributes = instanceAttr.data();

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
            mousePos: vec2<f32>,
            dragStart: vec2<f32>,
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

    wgpu::ShaderModule shaderModule = app->device.CreateShaderModule(&shaderDesc);

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
    colorTarget.format = app->colorFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    wgpu::VertexState vertexState;
    vertexState.module = shaderModule;
    vertexState.entryPoint = "vs_main";
    vertexState.bufferCount = static_cast<uint32_t>(vertexBufLayout.size());
    vertexState.constantCount = 0;
    vertexState.buffers = vertexBufLayout.data();

    // Create global bind group layout
    std::array<wgpu::BindGroupLayoutEntry, 2> globalGroupLayoutEntries;
    globalGroupLayoutEntries[0].binding = 0;
    globalGroupLayoutEntries[0].visibility =
        wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Compute;
    globalGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
    globalGroupLayoutEntries[0].buffer.type = wgpu::BufferBindingType::Uniform;
    globalGroupLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    globalGroupLayoutEntries[1].binding = 1;
    globalGroupLayoutEntries[1].visibility =
        wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
    globalGroupLayoutEntries[1].buffer.hasDynamicOffset = false;
    globalGroupLayoutEntries[1].buffer.type = wgpu::BufferBindingType::Storage;

    wgpu::BindGroupLayoutDescriptor globalGroupLayoutDesc;
    globalGroupLayoutDesc.entryCount = static_cast<uint32_t>(globalGroupLayoutEntries.size());
    globalGroupLayoutDesc.entries = globalGroupLayoutEntries.data();

    wgpu::BindGroupLayout globalGroupLayout =
        app->device.CreateBindGroupLayout(&globalGroupLayoutDesc);

    // Create main pipeline
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &globalGroupLayout;

    wgpu::PipelineLayout pipelineLayout =
        app->device.CreatePipelineLayout(&pipelineLayoutDesc);

    wgpu::RenderPipelineDescriptor renderPipelineDesc;
    renderPipelineDesc.label = "Canvas";
    renderPipelineDesc.vertex = vertexState;
    renderPipelineDesc.fragment = &fragmentState;
    renderPipelineDesc.layout = pipelineLayout;
    renderPipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    renderPipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
    renderPipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
    renderPipelineDesc.multisample.count = 1;
    renderPipelineDesc.multisample.mask = ~0u;
    renderPipelineDesc.multisample.alphaToCoverageEnabled = false;

    app->mainPipeline = app->device.CreateRenderPipeline(&renderPipelineDesc);

    // Create buffers
    wgpu::BufferDescriptor uboBufDesc;
    uboBufDesc.mappedAtCreation = false;
    uboBufDesc.size = sizeof(Uniforms);
    uboBufDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    app->viewParamBuf = app->device.CreateBuffer(&uboBufDesc);

    wgpu::BufferDescriptor layerBufDesc;
    layerBufDesc.mappedAtCreation = false;
    layerBufDesc.size = NumLayers * sizeof(mc::Layer);
    layerBufDesc.usage =
        wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage;
    app->layerBuf = app->device.CreateBuffer(&layerBufDesc);

    wgpu::BufferDescriptor selectionOutputBufDesc;
    selectionOutputBufDesc.mappedAtCreation = false;
    selectionOutputBufDesc.size = sizeof(float) * NumLayers;
    selectionOutputBufDesc.usage =
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    app->selectionBuf = app->device.CreateBuffer(&selectionOutputBufDesc);

    selectionOutputBufDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    app->selectionMapBuf = app->device.CreateBuffer(&selectionOutputBufDesc);

    // Create the bind group for the uniform and shared buffers
    std::array<wgpu::BindGroupEntry, 2> globalGroupEntries;
    globalGroupEntries[0].binding = 0;
    globalGroupEntries[0].buffer = app->viewParamBuf;
    globalGroupEntries[0].size = uboBufDesc.size;

    globalGroupEntries[1].binding = 1;
    globalGroupEntries[1].buffer = app->selectionBuf;
    globalGroupEntries[1].offset = 0;
    globalGroupEntries[1].size = selectionOutputBufDesc.size;

    wgpu::BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = globalGroupLayout;
    bindGroupDesc.entryCount = static_cast<uint32_t>(globalGroupEntries.size());
    bindGroupDesc.entries = globalGroupEntries.data();

    app->bindGroup = app->device.CreateBindGroup(&bindGroupDesc);

    // Create layer quad vertex buffer
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.mappedAtCreation = true;
    bufferDesc.size = mc::VertexData.size() * sizeof(float);
    bufferDesc.usage = wgpu::BufferUsage::Vertex;
    app->vertexBuf = app->device.CreateBuffer(&bufferDesc);
    std::memcpy(app->vertexBuf.GetMappedRange(), mc::VertexData.data(), bufferDesc.size);
    app->vertexBuf.Unmap();

    // Set up compute shader used to compute selection
    const std::string computeShaderSource = R"(
        struct Uniforms {
            proj: mat4x4<f32>,
            mousePos: vec2<f32>,
            dragStart: vec2<f32>,
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

            let minX = min(uniforms.mousePos.x, uniforms.dragStart.x);
            let minY = min(uniforms.mousePos.y, uniforms.dragStart.y);
            let maxX = max(uniforms.mousePos.x, uniforms.dragStart.x);
            let maxY = max(uniforms.mousePos.y, uniforms.dragStart.y);

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

    wgpu::ShaderModule computeShaderModule =
        app->device.CreateShaderModule(&computeShaderModuleDesc);

    std::array<wgpu::BindGroupLayoutEntry, 1> computeGroupLayoutEntries;

    computeGroupLayoutEntries[0].binding = 0;
    computeGroupLayoutEntries[0].visibility = wgpu::ShaderStage::Compute;
    computeGroupLayoutEntries[0].buffer.hasDynamicOffset = false;
    computeGroupLayoutEntries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    computeGroupLayoutEntries[0].buffer.minBindingSize = sizeof(mc::Layer);

    wgpu::BindGroupLayoutDescriptor computeBindGroupLayoutDesc;
    computeBindGroupLayoutDesc.entryCount =
        static_cast<uint32_t>(computeGroupLayoutEntries.size());
    computeBindGroupLayoutDesc.entries = computeGroupLayoutEntries.data();

    wgpu::BindGroupLayout computeGroupLayout =
        app->device.CreateBindGroupLayout(&computeBindGroupLayoutDesc);

    std::array<wgpu::BindGroupLayout, 2> computeBindGroupLayouts = {globalGroupLayout,
                                                                    computeGroupLayout};

    wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc;
    computePipelineLayoutDesc.bindGroupLayoutCount =
        static_cast<uint32_t>(computeBindGroupLayouts.size());
    computePipelineLayoutDesc.bindGroupLayouts = computeBindGroupLayouts.data();

    wgpu::PipelineLayout computePipelineLayout =
        app->device.CreatePipelineLayout(&computePipelineLayoutDesc);

    wgpu::ComputePipelineDescriptor computePipelineDesc;
    computePipelineDesc.label = "Canvas Selection";
    computePipelineDesc.layout = computePipelineLayout;
    computePipelineDesc.compute.module = computeShaderModule;
    computePipelineDesc.compute.entryPoint = "cs_main";

    app->selectionPipeline = app->device.CreateComputePipeline(&computePipelineDesc);

    std::array<wgpu::BindGroupEntry, 1> computeGroupEntries;

    computeGroupEntries[0].binding = 0;
    computeGroupEntries[0].buffer = app->layerBuf;
    computeGroupEntries[0].offset = 0;
    computeGroupEntries[0].size = layerBufDesc.size;

    wgpu::BindGroupDescriptor computeBindGroupDesc;
    computeBindGroupDesc.layout = computeGroupLayout;
    computeBindGroupDesc.entryCount = static_cast<uint32_t>(computeGroupEntries.size());
    computeBindGroupDesc.entries = computeGroupEntries.data();

    app->selectionBindGroup = app->device.CreateBindGroup(&computeBindGroupDesc);
}

void initUI(AppContext *app)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // configure fonts
    ImFontConfig configRoboto;
    configRoboto.FontDataOwnedByAtlas = false;
    configRoboto.OversampleH = 2;
    configRoboto.OversampleV = 2;
    configRoboto.RasterizerMultiply = 1.5f;
    configRoboto.GlyphExtraSpacing = ImVec2(1.0f, 0);
    io.Fonts->AddFontFromMemoryTTF(
        const_cast<uint8_t *>(Roboto_ttf), Roboto_ttf_size, 18.0f, &configRoboto);

    ImFontConfig configLucide;
    configLucide.FontDataOwnedByAtlas = false;
    configLucide.OversampleH = 2;
    configLucide.OversampleV = 2;
    configLucide.MergeMode = true;
    configLucide.GlyphMinAdvanceX = 24.0f;  // Use if you want to make the icon monospaced
    configLucide.GlyphOffset = ImVec2(0.0f, 5.0f);

    // Specify which icons we use
    // Need to specify or texture atlas will be too large and fail to upload to gpu on
    // lower systems
    ImFontGlyphRangesBuilder builder;
    builder.AddText(ICON_LC_GITHUB);
    builder.AddText(ICON_LC_IMAGE_UP);
    builder.AddText(ICON_LC_IMAGE_DOWN);
    builder.AddText(ICON_LC_ROTATE_CW);
    builder.AddText(ICON_LC_ARROW_UP_NARROW_WIDE);
    builder.AddText(ICON_LC_ARROW_DOWN_NARROW_WIDE);
    builder.AddText(ICON_LC_FLIP_HORIZONTAL_2);
    builder.AddText(ICON_LC_FLIP_VERTICAL_2);
    builder.AddText(ICON_LC_BRUSH);
    builder.AddText(ICON_LC_SQUARE_DASHED_MOUSE_POINTER);
    builder.AddText(ICON_LC_TRASH_2);
    builder.AddText(ICON_LC_HAND);
    builder.AddText(ICON_LC_MOUSE_POINTER);
    builder.AddText(ICON_LC_UNDO);
    builder.AddText(ICON_LC_REDO);
    builder.AddText(ICON_LC_TYPE);
    builder.AddText(ICON_LC_INFO);
    builder.AddText(ICON_LC_CROP);

    ImVector<ImWchar> iconRanges;
    builder.BuildRanges(&iconRanges);

    io.Fonts->AddFontFromMemoryTTF(const_cast<uint8_t *>(Lucide_ttf),
                                   Lucide_ttf_size,
                                   24.0f,
                                   &configLucide,
                                   iconRanges.Data);

    io.Fonts->Build();

    // add style
    ImGuiStyle &style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 1.0f;
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.WindowRounding = 3.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(20.0f, 20.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ChildRounding = 3.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 3.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(6.0f, 6.0f);
    style.FrameRounding = 4.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(12.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 3.0f);
    style.CellPadding = ImVec2(12.0f, 6.0f);
    style.IndentSpacing = 20.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabMinSize = 12.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.TabBorderSize = 0.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549295425415f, 0.5529412031173706f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.2901960909366608f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.9960784316062927f, 0.4745098054409027f, 0.6980392336845398f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
}

void drawUI(AppContext *app, const wgpu::RenderPassEncoder &renderPass)
{
    static bool show_test_window = true;
    static bool show_another_window = false;
    static bool show_quit_dialog = false;
    static float f = 0.0f;

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    ImGui::NewFrame();

    ImGui::Text("width: %d, height: %d, dpi: %.1f scale:%.1f\n pos x:%.1f\n pos y:%.1f\n",
                app->width,
                app->height,
                static_cast<float>(app->width) / static_cast<float>(app->bbwidth),
                app->viewParams.scale,
                app->viewParams.mousePos.x,
                app->viewParams.mousePos.y);
    ImGui::Text("NOTE: programmatic quit isn't supported on mobile");
    if (ImGui::Button("Hard Quit"))
    {
        app->appQuit = true;
    }
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowDemoWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow();
    }

    // 4. Prepare and conditionally open the "Really Quit?" popup
    if (ImGui::BeginPopupModal("Really Quit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Do you really want to quit?\n");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            app->appQuit = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (show_quit_dialog)
    {
        ImGui::OpenPopup("Really Quit?");
        show_quit_dialog = false;
    }

    ImGui::Begin("toolbox", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
    {
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::SetWindowSize(ImVec2(80, 300));

        ImGui::PushID("Upload Image Button");
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
        if (ImGui::Button(ICON_LC_IMAGE_UP, ImVec2(50, 50))) {
            // upload image code goes goes here
        }
        ImGui::PopStyleColor(1);
        ImGui::PopID();

        std::array<std::string, 4> tools = {
            ICON_LC_MOUSE_POINTER, ICON_LC_BRUSH, ICON_LC_TYPE, ICON_LC_HAND};
        std::array<State, 4> states = {Cursor, Paint, Text, Pan};

        for (size_t i = 0; i < tools.size(); i++) {
            ImGui::PushID(i);
            ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Button];
            if (app->state == states[i]) {
                color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
            }

            ImGui::PushStyleColor(ImGuiCol_Button, color);
            if (ImGui::Button(tools[i].c_str(), ImVec2(50, 50)) && states[i] != Other) {
                app->state = states[i];
            }
            ImGui::PopStyleColor(1);
            ImGui::PopID();
        }
    }
    ImGui::End();

    if (app->state == Cursor && app->mouseDown && app->mouseDragStart != app->mouseWindowPos) {
        ImDrawList *drawList = ImGui::GetBackgroundDrawList();
        drawList->AddRect(ImVec2(app->mouseDragStart.x, app->mouseDragStart.y),
                          ImVec2(app->mouseWindowPos.x, app->mouseWindowPos.y),
                          ImGui::GetColorU32(IM_COL32(0, 130, 216, 255)));
        drawList->AddRectFilled(ImVec2(app->mouseDragStart.x, app->mouseDragStart.y),
                                ImVec2(app->mouseWindowPos.x, app->mouseWindowPos.y),
                                ImGui::GetColorU32(IM_COL32(0, 130, 216, 50)));
    }

    ImGui::EndFrame();

    ImGui::Render();

    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());
}

bool initSwapChain(AppContext *app) {
    SDL_GetWindowSize(app->window, &app->width, &app->height);
    SDL_GetWindowSizeInPixels(app->window, &app->bbwidth, &app->bbheight);

    wgpu::SwapChainDescriptor swapChainDesc;
    swapChainDesc.width = static_cast<uint32_t>(app->width);
    swapChainDesc.height = static_cast<uint32_t>(app->height);

    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment,
    swapChainDesc.format = app->colorFormat;
    swapChainDesc.presentMode = wgpu::PresentMode::Fifo;

    app->swapchain = app->device.CreateSwapChain(app->surface, &swapChainDesc);

    app->updateView = true;

    return true;
}

wgpu::Device requestDevice(const wgpu::Adapter &adapter,
                           const wgpu::DeviceDescriptor *descriptor)
{
    struct UserData {
        wgpu::Device device;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status,
                                   WGPUDevice device,
                                   char const *message,
                                   void *pUserData) {
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = wgpu::Device::Acquire(device);
        } else {
            SDL_Log("Could not get WebGPU device: %s", message);
        }
        userData.requestEnded = true;
    };

    adapter.RequestDevice(
        descriptor, onDeviceRequestEnded, reinterpret_cast<void *>(&userData));

    // request device is async on web so hacky solution for now is to sleep
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    emscripten_sleep(100);
#endif

    return std::move(userData.device);
}

wgpu::Adapter requestAdapter(const wgpu::Instance &instance,
                             const wgpu::RequestAdapterOptions *options)
{
    struct UserData {
        wgpu::Adapter adapter;
        bool requestEnded = false;
    };
    UserData userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status,
                                    WGPUAdapter adapter,
                                    char const *message,
                                    void *pUserData) {
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = wgpu::Adapter::Acquire(adapter);
        } else {
            SDL_Log("Could not get WebGPU adapter %s", message);
        }
        userData.requestEnded = true;
    };

    instance.RequestAdapter(options, onAdapterRequestEnded, (void *)&userData);

    // request adapter is async on web so hacky solution for now is to sleep
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    emscripten_sleep(100);
#endif

    return std::move(userData.adapter);
}

int SDL_Fail()
{
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return -1;
}

int SDL_AppInit(void **appstate, int argc, char *argv[])
{
    AppContext* app = new AppContext;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        return SDL_Fail();
    }

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    app->window = SDL_CreateWindow("Miskeenity Canvas", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    if (!app->window) {
        return SDL_Fail();
    }

    app->instance = wgpu::CreateInstance();
    if (!app->instance) {
        return SDL_Fail();
    }

    app->surface = SDL_GetWGPUSurface(app->instance, app->window);
    if (!app->surface.Get()) {
        return SDL_Fail();
    }

    wgpu::RequestAdapterOptions adapterOpts;
    adapterOpts.compatibleSurface = app->surface;

    app->adapter = requestAdapter(app->instance, &adapterOpts);
    if (!app->adapter) {
        return SDL_Fail();
    }

    wgpu::SupportedLimits supportedLimits;
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    // Error in Chrome: Aborted(TODO: wgpuAdapterGetLimits unimplemented)
    // (as of September 4, 2023), so we hardcode values:
    // These work for 99.95% of clients (source: https://web3dsurvey.com/webgpu)
    supportedLimits.limits.minStorageBufferOffsetAlignment = 256;
    supportedLimits.limits.minUniformBufferOffsetAlignment = 256;
#else
    app->adapter.GetLimits(&supportedLimits);
#endif

    wgpu::RequiredLimits requiredLimits;
    requiredLimits.limits.maxVertexAttributes = 4;
    requiredLimits.limits.maxVertexBuffers = 2;
    // requiredLimits.limits.maxBufferSize = 150000 * sizeof(WGPUVertexAttributes);
    // requiredLimits.limits.maxVertexBufferArrayStride = sizeof(WGPUVertexAttributes);
    requiredLimits.limits.minStorageBufferOffsetAlignment =
        supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.minUniformBufferOffsetAlignment =
        supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 8;
    requiredLimits.limits.maxBindGroups = 4;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
    // Allow textures up to 2K
    requiredLimits.limits.maxTextureDimension1D = 2048;
    requiredLimits.limits.maxTextureDimension2D = 2048;
    requiredLimits.limits.maxTextureArrayLayers = 1;
    requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
    requiredLimits.limits.maxSamplersPerShaderStage = 1;
    requiredLimits.limits.maxComputeWorkgroupSizeX = WorkGroupSize;
    requiredLimits.limits.maxComputeWorkgroupSizeY = WorkGroupSize;
    requiredLimits.limits.maxComputeWorkgroupSizeZ = WorkGroupSize;

    wgpu::DeviceDescriptor deviceDesc;
    deviceDesc.label = "Device";
    // deviceDesc.requiredLimits = &requiredLimits;
    deviceDesc.defaultQueue.label = "Main Queue";
    app->device = requestDevice(app->adapter, &deviceDesc);

#if defined(SDL_PLATFORM_EMSCRIPTEN)
    app->colorFormat = app->surface.GetPreferredFormat(app->adapter);
#else()
    app->colorFormat = wgpu::TextureFormat::BGRA8Unorm;
#endif()

    app->device.SetUncapturedErrorCallback(
        [](WGPUErrorType type, char const *message, void *userData) {
            SDL_Log("Device error type: %d\n", type);
            SDL_Log("Device error message: %s\n", message);

            AppContext *app = static_cast<AppContext *>(userData);
            app->appQuit = true;
        },
        app);

    initSwapChain(app);

    initUI(app);

    ImGui_ImplSDL3_InitForOther(app->window);

    ImGui_ImplWGPU_InitInfo imguiWgpuInfo{};
    imguiWgpuInfo.Device = app->device.Get();
    imguiWgpuInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>(app->colorFormat);
    ImGui_ImplWGPU_Init(&imguiWgpuInfo);

    initMainPipeline(app);

    // print some information about the window
    SDL_ShowWindow(app->window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(app->window, &width, &height);
        SDL_GetWindowSizeInPixels(app->window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if (width != bbwidth) {
            SDL_Log("This is a highdpi environment.");
        }
    }

    app->updateView = true;

    *appstate = app;

    SDL_Log("Application started successfully!");

    return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event *event)
{
    AppContext *app = reinterpret_cast<AppContext *>(appstate);
    ImGuiIO& io = ImGui::GetIO();

    switch (event->type) {
    case SDL_EVENT_QUIT:
        app->appQuit = true;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        app->resetSwapchain = true;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        io.AddMouseButtonEvent(0, true);
        
        if(!io.WantCaptureMouse)
        {
            app->mouseDragStart = app->mouseWindowPos;
            app->mouseDown = true;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        io.AddMouseButtonEvent(0, false);

        // we want to place a sprite on click
        if (app->mouseWindowPos == app->mouseDragStart) {
            app->addLayer = true;
        }
        app->mouseDown = false;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        io.AddMousePosEvent(event->motion.x, event->motion.y);
        app->mouseWindowPos = glm::vec2(event->motion.x, event->motion.y);
        app->mouseDelta += glm::vec2(event->motion.xrel, event->motion.yrel);

        if(app->mouseDown && !io.WantCaptureMouse)
        {
            app->updateView = true;
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        io.AddMouseWheelEvent(event->wheel.x, event->wheel.y);
        app->scrollDelta += glm::vec2(event->wheel.x, event->wheel.y);

        if(!io.WantCaptureMouse)
        {
            app->updateView = true;
        }
        break;
    default:
        break;
    }

    return 0;
}

int SDL_AppIterate(void *appstate)
{
    AppContext *app = reinterpret_cast<AppContext *>(appstate);

    if (app->resetSwapchain) {
        if (!initSwapChain(app)) {
            return false;
        }

        app->resetSwapchain = false;
    }

    // Update canvas offset
    if (app->mouseDelta.length() > 0.0 && app->mouseDown && app->updateView &&
        app->state == Pan) {
        app->viewParams.canvasPos += app->mouseDelta;
    }

    // Update mouse position in canvas coordinate space
    app->viewParams.mousePos =
        (app->mouseWindowPos - app->viewParams.canvasPos) / app->viewParams.scale;

    // Update zoom level
    // For zoom, we want to center it around the mouse position
    if (app->scrollDelta.y != 0.0 && app->updateView) {
        float newScale =
            std::max<float>(1.0, app->scrollDelta.y * ZoomScaleFactor + app->viewParams.scale);
        float deltaScale = newScale - app->viewParams.scale;
        app->viewParams.scale = newScale;
        app->viewParams.canvasPos -= app->viewParams.mousePos * deltaScale;
    }

    if (app->updateView) {
        app->viewParams.width = app->width;
        app->viewParams.height = app->height;

        float l = -app->viewParams.canvasPos.x * 1.0 / app->viewParams.scale;
        float r = (app->width - app->viewParams.canvasPos.x) * 1.0 / app->viewParams.scale;
        float t = -app->viewParams.canvasPos.y * 1.0 / app->viewParams.scale;
        float b = (app->height - app->viewParams.canvasPos.y) * 1.0 / app->viewParams.scale;

        app->viewParams.proj = glm::mat4(   2.0/(r-l),  0.0,        0.0, (r+l)/(l-r),
                                            0.0,        2.0/(t-b),  0.0, (t+b)/(b-t),
                                            0.0,        0.0,        0.5, 0.5,
                                            0.0,        0.0,        0.0, 1.0);
    }

    if (app->state == Cursor && app->mouseDown && app->mouseDragStart != app->mouseWindowPos) {
        app->selectionRequested = true;
    }

    app->device.GetQueue().WriteBuffer(
        app->viewParamBuf, 0, &app->viewParams, sizeof(Uniforms));

    if (app->addLayer && app->state == Cursor) {
        // temporary generate random color
        const uint32_t a = 1664525;
        const uint32_t c = 1013904223;

        const uint32_t color = a * app->layers.length() + c;
        const uint8_t red = static_cast<uint8_t>((color >> 16) & 0xFF);
        const uint8_t green = static_cast<uint8_t>((color >> 8) & 0xFF);
        const uint8_t blue = static_cast<uint8_t>(color & 0xFF);

        app->layers.add(
            {(app->mouseWindowPos - app->viewParams.canvasPos) / app->viewParams.scale,
             glm::vec2(100, 0),
             glm::vec2(0, 100),
             glm::u16vec2(0),
             glm::u16vec2(0),
             0,
             0,
             glm::u8vec3(red, green, blue),
             0});

        app->layersModified = true;
    }

    if (app->layersModified) {
        app->device.GetQueue().WriteBuffer(
            app->layerBuf, 0, app->layers.data(), app->layers.length() * sizeof(mc::Layer));
    }

    wgpu::TextureView nextTexture = app->swapchain.GetCurrentTextureView();
    // Getting the texture may fail, in particular if the window has been resized
    // and thus the target surface changed.
    if (!nextTexture) {
        SDL_Log("Cannot acquire next swap chain texture");
        return false;
    }

    wgpu::CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Casper";

    wgpu::CommandEncoder encoder = app->device.CreateCommandEncoder(&commandEncoderDesc);

    wgpu::RenderPassColorAttachment renderPassColorAttachment;
    renderPassColorAttachment.view = nextTexture,
    renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear,
    renderPassColorAttachment.storeOp = wgpu::StoreOp::Store,
    renderPassColorAttachment.clearValue = wgpu::Color{0.949f, 0.929f, 0.898f, 1.0f};

    wgpu::RenderPassDescriptor renderPassDesc;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    wgpu::RenderPassEncoder renderPassEnc = encoder.BeginRenderPass(&renderPassDesc);

    if (app->layers.length() > 0) {
        renderPassEnc.SetPipeline(app->mainPipeline);
        renderPassEnc.SetVertexBuffer(0, app->vertexBuf);
        renderPassEnc.SetVertexBuffer(1, app->layerBuf);
        renderPassEnc.SetBindGroup(0, app->bindGroup);
        renderPassEnc.Draw(6, app->layers.length(), 0, 0);
    }

    drawUI(app, renderPassEnc);

    renderPassEnc.End();

    // We only want to recompute the selection array if a selection is requested and any
    // previously requested computations have completed aka selection ready
    if (app->selectionRequested && app->selectionReady && app->layers.length() > 0) {
        encoder.ClearBuffer(app->selectionBuf, 0, app->layers.length() * sizeof(uint32_t));
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline(app->selectionPipeline);
        // figure out which things to bind;
        computePass.SetBindGroup(0, app->bindGroup);
        computePass.SetBindGroup(1, app->selectionBindGroup);

        computePass.DispatchWorkgroups((app->layers.length() + 256 - 1) / 256, 1, 1);
        computePass.End();

        app->selectionMapBuf.Unmap();
        app->selectionFlags = nullptr;

        encoder.CopyBufferToBuffer(
            app->selectionBuf, 0, app->selectionMapBuf, 0, sizeof(float) * NumLayers);
    }

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label = "Melchior";
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);

    app->device.GetQueue().Submit(1, &command);

    if (app->selectionRequested && app->selectionReady) {
        app->selectionReady = false;
        wgpu::BufferMapCallback callback = [](WGPUBufferMapAsyncStatus status,
                                              void *userdata) {
            if (status == WGPUBufferMapAsyncStatus_Success) {
                AppContext *app = reinterpret_cast<AppContext *>(userdata);
                app->selectionFlags = reinterpret_cast<uint32_t *>(
                    const_cast<void *>((app->selectionMapBuf.GetConstMappedRange(
                        0, sizeof(float) * app->layers.length()))));
                app->selectionReady = true;
            }
        };
        app->selectionMapBuf.MapAsync(
            wgpu::MapMode::Read, 0, sizeof(float) * NumLayers, callback, app);
    }

#if !defined(SDL_PLATFORM_EMSCRIPTEN)
    app->swapchain.Present();
    app->device.Tick();
#endif

    // Reset
    app->mouseDelta = glm::vec2(0.0);
    app->scrollDelta = glm::vec2(0.0);
    app->updateView = false;
    app->layersModified = false;
    app->addLayer = false;
    app->selectionRequested = false;

    return app->appQuit;
}

void SDL_AppQuit(void *appstate)
{
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    AppContext *app = reinterpret_cast<AppContext *>(appstate);
    if (app) {
        SDL_DestroyWindow(app->window);
        delete app;
    }

    SDL_Quit();
    SDL_Log("Application quit successfully!");
}
