#pragma once

#include "layers.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

namespace mc
{

    constexpr float ZoomScaleFactor = 0.5;
    constexpr size_t NumLayers      = 2048;

    enum class State
    {
        Cursor,
        Pan,
        Paint,
        Text,
        Other
    };

    enum class SelectDispatch : uint32_t
    {
        Box         = 0,
        Point       = 1,
        ComputeBbox = 2,
        None        = 3
    };

    enum class CursorDragType
    {
        Select,
        Move,
        Rotate,
        ScaleTL,
        ScaleBR,
        ScaleTR,
        ScaleBL
    };

#pragma pack( push )
    struct Uniforms
    {
        glm::mat4 proj;
        glm::vec2 canvasPos           = glm::vec2( 0.0 );
        glm::vec2 mousePos            = glm::vec2( 0.0 );
        glm::vec2 mouseSelectPos      = glm::vec2( 0.0 );
        SelectDispatch selectDispatch = SelectDispatch::None;
        uint32_t width;
        uint32_t height;
        float scale = 1.0;

        float _pad[6];
    };
#pragma pack( pop )
    // Have the compiler check byte alignment
    // Total size must be a multiple of the alignment size of its largest element
    static_assert( sizeof( Uniforms ) % sizeof( glm::mat4 ) == 0 );

    const std::vector<float> SquareVertexData{
        -0.5, -0.5, 0.0, 1.0, +0.5, -0.5, 1.0, 1.0, +0.5, +0.5, 0.0, 0.0,

        -0.5, -0.5, 0.0, 1.0, +0.5, +0.5, 0.0, 0.0, -0.5, +0.5, 1.0, 0.0,
    };

    struct AppContext
    {
        SDL_Window* window;

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

        State state             = State::Cursor;
        CursorDragType dragType = CursorDragType::Select;
        bool mouseDown          = false;
        bool updateView         = false;
        bool selectionReady     = true;
        bool layersModified     = true;
        bool addLayer           = false;
        bool appQuit            = false;
        bool resetSwapchain     = false;

        Selection* selectionData = nullptr;

        // Input variables
        glm::vec2 mouseWindowPos  = glm::vec2( 0.0 );
        glm::vec2 mouseDragStart  = glm::vec2( 0.0 );
        glm::vec2 mouseDelta      = glm::vec2( 0.0 );
        glm::vec2 scrollDelta     = glm::vec2( 0.0 );
        glm::vec4 selectionBbox   = glm::vec4( 0.0 );
        glm::vec2 selectionCenter = glm::vec2( 0.0 );

        Layers layers = mc::Layers( NumLayers );
    };

} // namespace mc