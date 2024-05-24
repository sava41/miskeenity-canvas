#pragma once

#include "layers.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <webgpu/webgpu.h>
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

#pragma pack( push )
    struct Uniforms
    {
        glm::mat4 proj;
        glm::vec2 canvasPos      = glm::vec2( 0.0 );
        glm::vec2 mousePos       = glm::vec2( 0.0 );
        glm::vec2 mouseSelectPos = glm::vec2( 0.0 );
        uint32_t width;
        uint32_t height;
        float scale = 1.0;
        float _pad[7];
    };
#pragma pack( pop )
    // Have the compiler check byte alignment
    // Total size must be a multiple of the alignment size of its largest field
    static_assert( sizeof( Uniforms ) % sizeof( glm::mat4 ) == 0 );

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
        bool updateView         = false;
        bool selectionRequested = false;
        bool selectionReady     = true;
        bool layersModified     = true;
        bool addLayer           = false;
        bool appQuit            = false;
        bool resetSwapchain     = false;

        uint32_t* selectionFlags = nullptr;

        // Input variables
        glm::vec2 mouseWindowPos = glm::vec2( 0.0 );
        glm::vec2 mouseDragStart = glm::vec2( 0.0 );
        glm::vec2 mouseDelta     = glm::vec2( 0.0 );
        glm::vec2 scrollDelta    = glm::vec2( 0.0 );
        bool mouseDown           = false;

        Layers layers = mc::Layers( NumLayers );
    };

} // namespace mc