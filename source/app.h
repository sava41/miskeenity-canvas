#pragma once

#include "layer_manager.h"
#include "mesh_manager.h"
#include "texture_manager.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

namespace mc
{

    constexpr float ZoomScaleFactor             = 0.3;
    constexpr size_t NumLayers                  = 2048;
    constexpr unsigned long resetSurfaceDelayMs = 150;

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
        float scale        = 1.0;
        uint32_t numLayers = 0;

        float _pad[5];
    };
#pragma pack( pop )
    // Have the compiler check byte alignment
    // Total size must be a multiple of the alignment size of its largest element
    static_assert( sizeof( Uniforms ) % sizeof( glm::mat4 ) == 0 );

    struct AppContext
    {
        SDL_Window* window;

        int width;
        int height;
        int bbwidth;
        int bbheight;
        float dpiFactor = 1.0f;

        wgpu::Instance instance;
        wgpu::Surface surface;
        wgpu::Device device;
        wgpu::Adapter adapter;

        wgpu::TextureFormat colorFormat;
        uint64_t maxVertexBufferSize;

        wgpu::RenderPipeline mainPipeline;
        wgpu::ComputePipeline selectionPipeline;
        wgpu::ComputePipeline meshPipeline;
        wgpu::Buffer meshBuf;
        wgpu::Buffer vertexBuf;
        wgpu::Buffer layerBuf;
        wgpu::Buffer viewParamBuf;
        wgpu::Buffer selectionBuf;
        wgpu::Buffer selectionMapBuf;
        wgpu::BindGroup globalBindGroup;
        wgpu::BindGroup selectionBindGroup;
        wgpu::BindGroup meshBindGroup;

        Uniforms viewParams;

        CursorDragType dragType        = CursorDragType::Select;
        bool mouseDown                 = false;
        bool updateView                = false;
        bool selectionReady            = true;
        bool layersModified            = true;
        bool appQuit                   = false;
        bool resetSurface              = false;
        unsigned long resetSurfaceTime = 0;

        Selection* selectionData = nullptr;

        // Input variables
        glm::vec2 mouseWindowPos  = glm::vec2( 0.0 );
        glm::vec2 mouseDragStart  = glm::vec2( 0.0 );
        glm::vec2 mouseDelta      = glm::vec2( 0.0 );
        glm::vec2 scrollDelta     = glm::vec2( 0.0 );
        glm::vec4 selectionBbox   = glm::vec4( 0.0 );
        glm::vec2 selectionCenter = glm::vec2( 0.0 );

        LayerManager layers           = LayerManager( NumLayers );
        TextureManager textureManager = TextureManager( 100 );
        MeshManager meshManager       = MeshManager();
        int layerEditStart            = 0;
    };

} // namespace mc