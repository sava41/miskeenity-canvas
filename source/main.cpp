#include <array>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <webgpu/webgpu.h>
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_sdl3.h>

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include "webgpu_surface.h"
#include "lucide.h"
#include "embedded_files.h"

struct AppContext {
    SDL_Window *window;
    WGPUInstance wgpu_instance;
    WGPUSurface wgpu_surface;
    WGPUTextureFormat colorFormat;
    WGPUDevice wgpu_device;
    WGPUAdapter wgpu_adapter;
    WGPUQueue wgpu_queue;
    WGPUSwapChain wgpu_swapchain;
    WGPURenderPipeline canvas_pipeline;
    SDL_bool app_quit = SDL_FALSE;
    SDL_bool reset_swapchain = SDL_FALSE;

    float bacgkround_color[4] = {0.949f, 0.929f, 0.898f, 1.0f};
};

void initCanvasPipeline(AppContext *app)
{
    const char *shaderSource = R"(
        @vertex
        fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
            var p = vec2f(0.0, 0.0);
            if (in_vertex_index == 0u) {
                p = vec2f(-0.5, -0.5);
            } else if (in_vertex_index == 1u) {
                p = vec2f(0.5, -0.5);
            } else {
                p = vec2f(0.0, 0.5);
            }
            return vec4f(p, 0.0, 1.0);
        }

        @fragment
        fn fs_main() -> @location(0) vec4f {
            return vec4f(0.0, 0.4, 1.0, 1.0);
        }
        )";

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = nullptr;

    // Use the extension mechanism to load a WGSL shader source code
    WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
    // Set the chained struct's header
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    // Setup the actual payload of the shader code descriptor
    shaderCodeDesc.code = shaderSource;

    WGPUShaderModule shaderModule =
        wgpuDeviceCreateShaderModule(app->wgpu_device, &shaderDesc);
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.nextInChain = nullptr;
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;

    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;

    WGPUFragmentState fragmentState = {};
    fragmentState.nextInChain = nullptr;
    pipelineDesc.fragment = &fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    WGPUBlendState blendState;
    // Usual alpha blending for the color:
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
    // We leave the target alpha untouched:
    blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTarget = {};
    colorTarget.nextInChain = nullptr;
    colorTarget.format = app->colorFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.depthStencil = nullptr;
    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;
    pipelineDesc.layout = nullptr;

    app->canvas_pipeline = wgpuDeviceCreateRenderPipeline(app->wgpu_device, &pipelineDesc);
}

void initUI(AppContext *app)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // configure fonts
    ImFontConfig configRoboto;
    configRoboto.FontDataOwnedByAtlas = false;
    configRoboto.OversampleH = 2;
    configRoboto.OversampleV = 2;
    configRoboto.RasterizerMultiply = 1.5f;
    configRoboto.GlyphExtraSpacing = ImVec2(1.0f, 0);
    io.Fonts->AddFontFromMemoryTTF(const_cast<uint8_t*>(Roboto_ttf), Roboto_ttf_size, 18.0f, &configRoboto);

    ImFontConfig configLucide;
    configLucide.FontDataOwnedByAtlas = false;
    configLucide.OversampleH = 2;
    configLucide.OversampleV = 2;
    configLucide.MergeMode = true;
    configLucide.GlyphMinAdvanceX = 24.0f; // Use if you want to make the icon monospaced
    configLucide.GlyphOffset = ImVec2(0.0f, 5.0f);

    // Specify which icons we use
    // Need to specify or texture atlas will be too large and fail to upload to gpu on lower systems
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

    io.Fonts->AddFontFromMemoryTTF(const_cast<uint8_t*>( Lucide_ttf), Lucide_ttf_size, 24.0f, &configLucide, iconRanges.Data);

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

void drawUI(AppContext *app, WGPURenderPassEncoder renderPass)
{
    static bool show_test_window = true;
    static bool show_another_window = false;
    static bool show_quit_dialog = false;
    static float f = 0.0f;

    int width, height, bbwidth, bbheight;
    SDL_GetWindowSize(app->window, &width, &height);
    SDL_GetWindowSizeInPixels(app->window, &bbwidth, &bbheight);

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    ImGui::NewFrame();

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::ColorEdit3("clear color", static_cast<float *>(&app->bacgkround_color[0]));
    ImGui::Text(
        "width: %d, height: %d, dpi: %.1f\n", width, height, float(width) / float(bbwidth));
    ImGui::Text("NOTE: programmatic quit isn't supported on mobile");
    if (ImGui::Button("Hard Quit"))
    {
        app->app_quit = SDL_TRUE;
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
            app->app_quit = SDL_TRUE;
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

        std::array<std::string, 5> tools = {ICON_LC_IMAGE_UP, ICON_LC_BRUSH, ICON_LC_TYPE, ICON_LC_HAND, ICON_LC_MOUSE_POINTER};

        for (size_t i = 0; i < tools.size(); i++)
        {
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            ImGui::Button(tools[i].c_str(), ImVec2(50, 50));
            ImGui::PopStyleColor(1);
            ImGui::PopID();
        }
    }
    ImGui::End();

    ImGui::EndFrame();

    ImGui::Render();

    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

bool initSwapChain(AppContext *app) {

    int width, height;
    SDL_GetWindowSize(app->window, &width, &height);

#if defined(SDL_PLATFORM_EMSCRIPTEN)
    app->colorFormat = wgpuSurfaceGetPreferredFormat(app->wgpu_surface, app->wgpu_adapter);
#else()
    app->colorFormat = WGPUTextureFormat_BGRA8Unorm;
#endif()

    WGPUSwapChainDescriptor swapChainDesc = {};

    swapChainDesc.width = static_cast<uint32_t>(width);
    swapChainDesc.height = static_cast<uint32_t>(height);

    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.format = app->colorFormat;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;
    WGPUSwapChain swapchain = wgpuDeviceCreateSwapChain(app->wgpu_device, app->wgpu_surface, &swapChainDesc);

    if(!swapchain){
        return false;
    }

    app->wgpu_swapchain = swapchain;

    return true;
}

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const *descriptor)
{
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status,
                                   WGPUDevice device,
                                   char const *message,
                                   void *pUserData) {
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            SDL_Log("Could not get WebGPU device: %s", message);
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(
        adapter, descriptor, onDeviceRequestEnded, reinterpret_cast<void *>(&userData));

    // request device is async on web so hacky solution for now is to sleep
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    emscripten_sleep(300);
#endif

    return userData.device;
}

WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const *options)
{
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status,
                                    WGPUAdapter adapter,
                                    char const *message,
                                    void *pUserData) {
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            SDL_Log("Could not get WebGPU adapter %s", message);
        }
        userData.requestEnded = true;
    };

    wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded, (void *)&userData);

    // request adapter is async on web so hacky solution for now is to sleep
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    emscripten_sleep(100);
#endif

    return userData.adapter;
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

    app->window = SDL_CreateWindow("Miskeenity Canvas", 640, 480, SDL_WINDOW_RESIZABLE);
    if (!app->window) {
        return SDL_Fail();
    }

    app->wgpu_instance = wgpuCreateInstance(nullptr);
    if (!app->wgpu_instance) {
        return SDL_Fail();
    }

    app->wgpu_surface = SDL_GetWGPUSurface(app->wgpu_instance, app->window);
    if (!app->wgpu_surface) {
        return SDL_Fail();
    }

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = app->wgpu_surface;

    app->wgpu_adapter = requestAdapter(app->wgpu_instance, &adapterOpts);
    if (!app->wgpu_adapter) {
        return SDL_Fail();
    }

    WGPUSupportedLimits supportedLimits;
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    // Error in Chrome: Aborted(TODO: wgpuAdapterGetLimits unimplemented)
    // (as of September 4, 2023), so we hardcode values:
    // These work for 99.95% of clients (source: https://web3dsurvey.com/webgpu)
    supportedLimits.limits.minStorageBufferOffsetAlignment = 256;
    supportedLimits.limits.minUniformBufferOffsetAlignment = 256;
#else
    wgpuAdapterGetLimits(app->wgpu_adapter, &supportedLimits);
#endif

    WGPURequiredLimits requiredLimits = {};
    requiredLimits.limits.maxVertexAttributes = 4;
    requiredLimits.limits.maxVertexBuffers = 1;
    // requiredLimits.limits.maxBufferSize = 150000 * sizeof(WGPUVertexAttributes);
    // requiredLimits.limits.maxVertexBufferArrayStride = sizeof(WGPUVertexAttributes);
    requiredLimits.limits.minStorageBufferOffsetAlignment =
        supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.minUniformBufferOffsetAlignment =
        supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 8;
    requiredLimits.limits.maxBindGroups = 2;
    //                                    ^ This was a 1
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
    // Allow textures up to 2K
    requiredLimits.limits.maxTextureDimension1D = 2048;
    requiredLimits.limits.maxTextureDimension2D = 2048;
    requiredLimits.limits.maxTextureArrayLayers = 1;
    requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
    requiredLimits.limits.maxSamplersPerShaderStage = 1;

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "Device";
    deviceDesc.requiredLimits = &requiredLimits;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "Main Queue";
    app->wgpu_device = requestDevice(app->wgpu_adapter, &deviceDesc);

    app->wgpu_queue = wgpuDeviceGetQueue(app->wgpu_device);

    initSwapChain(app); 

    initUI(app);

    ImGui_ImplSDL3_InitForOther(app->window);

    ImGui_ImplWGPU_InitInfo imguiWgpuInfo{};
    imguiWgpuInfo.Device = app->wgpu_device;
    imguiWgpuInfo.RenderTargetFormat = app->colorFormat;
    ImGui_ImplWGPU_Init(&imguiWgpuInfo);

    initCanvasPipeline(app);

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

    *appstate = app;

    SDL_Log("Application started successfully!");

    return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event *event)
{
    AppContext *app = (AppContext *)appstate;
    ImGuiIO& io = ImGui::GetIO();

    switch (event->type) {
    case SDL_EVENT_QUIT:
        app->app_quit = SDL_TRUE;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        app->reset_swapchain = SDL_TRUE;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        io.AddMouseButtonEvent(0, true);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        io.AddMouseButtonEvent(0, false);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        io.AddMouseWheelEvent(event->wheel.x, event->wheel.y);
        break;
    default:
        break;
    }

    return 0;
}

int SDL_AppIterate(void *appstate)
{
    AppContext *app = (AppContext *)appstate;

    if (app->reset_swapchain || !app->wgpu_swapchain) {
        wgpuSwapChainRelease(app->wgpu_swapchain);

        if (!initSwapChain(app)) {
            return SDL_FALSE;
        }

        app->reset_swapchain = SDL_FALSE;
    }

    WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(app->wgpu_swapchain);
    // Getting the texture may fail, in particular if the window has been resized
    // and thus the target surface changed.
    if (!nextTexture) {
        SDL_Log("Cannot acquire next swap chain texture");
        return SDL_FALSE;
    }

    WGPUCommandEncoderDescriptor commandEncoderDesc = {};
    commandEncoderDesc.nextInChain = nullptr;
    commandEncoderDesc.label = "Casper";
    WGPUCommandEncoder encoder =
        wgpuDeviceCreateCommandEncoder(app->wgpu_device, &commandEncoderDesc);

    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{   app->bacgkround_color[0], 
                                                        app->bacgkround_color[1], 
                                                        app->bacgkround_color[2], 
                                                        app->bacgkround_color[3] 
                                                    };
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif();

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;
    renderPassDesc.nextInChain = nullptr;

    // Create a render pass. We end it immediately because we use its built-in
    // mechanism for clearing the screen when it begins (see descriptor).
    WGPURenderPassEncoder renderPass =
        wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    wgpuRenderPassEncoderSetPipeline(renderPass, app->canvas_pipeline);
    wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

    drawUI(app, renderPass);

    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

    wgpuTextureViewRelease(nextTexture);

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Melchior";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(app->wgpu_queue, 1, &command);
    wgpuCommandBufferRelease(command);

#if !defined(SDL_PLATFORM_EMSCRIPTEN)
    // We can tell the swap chain to present the next texture.
    wgpuSwapChainPresent(app->wgpu_swapchain);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(app->wgpu_device);
#endif

    return app->app_quit;
}

void SDL_AppQuit(void *appstate)
{
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();  
    ImGui::DestroyContext();

    auto *app = (AppContext *)appstate;
    if (app) {
        wgpuQueueRelease(app->wgpu_queue);
        wgpuSwapChainRelease(app->wgpu_swapchain);
        wgpuDeviceRelease(app->wgpu_device);
        wgpuAdapterRelease(app->wgpu_adapter);
        wgpuInstanceRelease(app->wgpu_instance);
        wgpuSurfaceRelease(app->wgpu_surface);
        SDL_DestroyWindow(app->window);
        delete app;
    }

    SDL_Quit();
    SDL_Log("Application quit successfully!");
}
