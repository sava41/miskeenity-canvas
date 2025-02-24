#pragma once

#include "font_manager.h"
#include "layer_history.h"
#include "layer_manager.h"
#include "mesh_manager.h"
#include "ml_inference.h"
#include "texture_manager.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

namespace mc
{

    const float ZoomScaleFactor             = 0.3;
    const size_t NumLayers                  = 2048;
    const size_t NumUndo                    = 100;
    const unsigned long resetSurfaceDelayMs = 150;

    const size_t MaxMeshBufferTriangles = std::numeric_limits<uint16_t>::max();
    constexpr size_t MaxMeshBufferSize  = MaxMeshBufferTriangles * sizeof( Triangle );

    enum class Mode
    {
        Cursor,
        Pan,
        Paint,
        Text,
        Crop,
        Cut,
        SegmentCut,
        Save
    };

    enum class SelectDispatch : uint32_t
    {
        Box         = 0,
        Point       = 1,
        ComputeBbox = 2,
        None        = 3
    };

    enum ViewFlags : uint32_t
    {
        SelectionRasterTarget  = 1 << 0,
        RenderSelectionOutline = 1 << 1,
    };

    enum class CursorDragType
    {
        None,
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
        uint32_t viewFlags;
        uint32_t width;
        uint32_t height;
        float scale        = 1.0;
        uint32_t numLayers = 0;
        float dpiScale     = 1.0;
        uint32_t ticks     = 0;

        float _pad[2];
    };
#pragma pack( pop )
    // Have the compiler check byte alignment
    // Total size must be a multiple of the alignment size of its largest element
    static_assert( sizeof( Uniforms ) % sizeof( glm::mat4 ) == 0 );

    struct AppContext
    {
        SDL_Window* window;

        Mode mode = Mode::Cursor;

        int width;
        int height;
        int bbwidth;
        int bbheight;
        float dpiFactor = 1.0f;
        uint64_t maxBufferSize;

        wgpu::Instance instance;
        wgpu::Surface surface;
        wgpu::Device device;
        wgpu::Adapter adapter;

        std::unique_ptr<mc::ResourceHandle> canvasRenderTextureHandle;
        std::unique_ptr<mc::ResourceHandle> canvasSelectMaskHandle;
        std::unique_ptr<mc::ResourceHandle> canvasSelectOccludedMaskHandle;
        wgpu::TextureFormat colorFormat;

        wgpu::RenderPipeline canvasPipeline;
        wgpu::RenderPipeline postPipeline;
        wgpu::RenderPipeline exportPipeline;
        wgpu::ComputePipeline selectionPipeline;
        wgpu::ComputePipeline meshPipeline;
        wgpu::ComputePipeline preAlphaPipeline;
        wgpu::ComputePipeline maskMultiplyPipeline;
        wgpu::ComputePipeline invMaskMultiplyPipeline;
        wgpu::ComputePipeline mipGenPipeline;

        wgpu::Buffer meshBuf;
        wgpu::Buffer vertexBuf;
        wgpu::Buffer vertexCopyBuf;
        wgpu::Buffer textureMapBuffer;
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
        bool mergeTopLayers            = false;
        bool appQuit                   = false;
        bool resetSurface              = false;
        bool updateUIStyles            = false;
        bool saveImage                 = false;
        bool rasterizeSelection        = false;
        unsigned long resetSurfaceTime = 0;

        // Input variables
        glm::vec2 mouseWindowPos  = glm::vec2( 0.0 );
        glm::vec2 mouseDragStart  = glm::vec2( 0.0 );
        glm::vec2 mouseDelta      = glm::vec2( 0.0 );
        glm::vec2 scrollDelta     = glm::vec2( 0.0 );
        glm::vec4 selectionAabb   = glm::vec4( 0.0 );
        glm::vec2 selectionCenter = glm::vec2( 0.0 );

        std::unique_ptr<mc::ResourceHandle> copyTextureHandle;
        std::unique_ptr<mc::ResourceHandle> editMaskTextureHandle;

        LayerManager layers           = LayerManager( NumLayers );
        LayerHistory layerHistory     = LayerHistory( NumUndo );
        TextureManager textureManager = TextureManager( 100 );
        MeshManager meshManager       = MeshManager( MaxMeshBufferTriangles );
        FontManager fontManager;
        int newMeshSize = 0;

        std::unique_ptr<mc::MlInference> mlInference;
    };

} // namespace mc