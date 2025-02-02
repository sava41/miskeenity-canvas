#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
#include <thread>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#if defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include "app.h"
#include "color_theme.h"
#include "events.h"
#include "graphics.h"
#include "image.h"
#include "layer_manager.h"
#include "sdl_utils.h"
#include "ui.h"
#include "webgpu_surface.h"

SDL_AppResult SDL_Fail()
{
    SDL_LogError( SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError() );
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit( void** appstate, int argc, char* argv[] )
{
    static mc::AppContext appContext;
    mc::AppContext* app = &appContext;
    *appstate           = app;

    if( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMEPAD ) )
    {
        return SDL_Fail();
    }

    SDL_SetHint( SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR, HTML_CANVAS_ID );

    app->window = SDL_CreateWindow( "Miskeenity Canvas", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIGH_PIXEL_DENSITY );
    if( !app->window )
    {
        return SDL_Fail();
    }
    mc::setWindowIcon( app->window );

    app->instance = wgpu::CreateInstance();
    if( !app->instance.Get() )
    {
        return SDL_Fail();
    }

    app->surface = wgpu::Surface( SDL_GetWGPUSurface( app->instance.Get(), app->window ) );
    if( !app->surface.Get() )
    {
        return SDL_Fail();
    }

    if( !mc::initDevice( app ) )
    {
        return SDL_Fail();
    }

    // setup default meshes
    if( app->maxBufferSize < app->meshManager.maxLength() * sizeof( mc::Triangle ) )
    {
        app->meshManager = mc::MeshManager( app->maxBufferSize / sizeof( mc::Triangle ) );
    }

    // add unit square mesh
    app->meshManager.add( { { { -0.5, -0.5, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                              { +0.5, -0.5, 1.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                              { +0.5, +0.5, 1.0, 1.0, 1.0, 1.0, 0xFFFFFFFF, 0 } },
                            { { -0.5, -0.5, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                              { +0.5, +0.5, 1.0, 1.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                              { -0.5, +0.5, 0.0, 1.0, 1.0, 1.0, 0xFFFFFFFF, 0 } } } );

    SDL_GetWindowSize( app->window, &app->width, &app->height );
    SDL_GetWindowSizeInPixels( app->window, &app->bbwidth, &app->bbheight );
    app->dpiFactor           = SDL_GetWindowDisplayScale( app->window );
    app->viewParams.dpiScale = SDL_GetWindowDisplayScale( app->window );

    mc::configureSurface( app );
    app->canvasRenderTextureHandle      = std::make_unique<mc::ResourceHandle>( app->textureManager.add(
        nullptr, app->bbwidth, app->bbheight, 4, app->device, wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding ) );
    app->canvasSelectMaskHandle         = std::make_unique<mc::ResourceHandle>( app->textureManager.add(
        nullptr, app->bbwidth, app->bbheight, 1, app->device, wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding ) );
    app->canvasSelectOccludedMaskHandle = std::make_unique<mc::ResourceHandle>( app->textureManager.add(
        nullptr, app->bbwidth, app->bbheight, 1, app->device, wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding ) );

    mc::initUI( app );

    mc::initPipelines( app );
    mc::initImageProcessingPipelines( app );

    if( SDL_ShowWindow( app->window ) )
    {
        SDL_Log( "Window size: %ix%i", app->width, app->height );
        SDL_Log( "Backbuffer size: %ix%i", app->bbwidth, app->bbheight );
    }

    app->textureManager.init( app->device );

    app->updateView = true;

    app->fontManager.init( app->textureManager, app->device, app->meshManager.getMeshInfo( mc::UnitSquareMeshIndex ) );

    app->mlInference = std::make_unique<mc::MlInference>( "sam_preprocess.onnx", "sam_vit_h_4b8939.onnx", std::thread::hardware_concurrency() );

    SDL_Log( "Application started successfully!" );

    return SDL_APP_CONTINUE;
}

void proccessUserEvent( const SDL_Event* sdlEvent, mc::AppContext* app )
{
    const mc::Event* eventData = reinterpret_cast<const mc::Event*>( sdlEvent );

    switch( static_cast<mc::Events>( sdlEvent->user.code ) )
    {
    case mc::Events::AppQuit:
        app->appQuit = true;
        break;
    case mc::Events::LoadImage:
        mc::loadImageFromFileDialog( app );
        break;
    case mc::Events::SaveImageRequest:
        app->saveImage = true;
        break;
    case mc::Events::SaveImage:
        mc::saveImageFromFileDialog( app );
        break;
    case mc::Events::OpenGithub:
        SDL_OpenURL( "https://github.com/sava41/miskeenity-canvas" );
        break;
    case mc::Events::SelectionChanged:
    {
        const mc::Selection* selectionData =
            reinterpret_cast<const mc::Selection*>( app->selectionMapBuf.GetConstMappedRange( 0, app->layers.getTotalTriCount() * sizeof( mc::Selection ) ) );

        if( selectionData == nullptr )
        {
            app->selectionMapBuf.Unmap();
            app->selectionReady            = true;
            app->viewParams.selectDispatch = mc::SelectDispatch::None;
            app->dragType                  = mc::CursorDragType::Select;
            return;
        }

        app->selectionAabb = glm::vec4( -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                        std::numeric_limits<float>::max() );

        int numSelected    = 0;
        int triangleOffset = app->layers.getTotalTriCount() - 1;

        for( int i = app->layers.length() - 1; i >= 0; --i )
        {
            // Calculate layer selection by checking each triangle in a layer
            bool boxSelected             = true;
            bool pointSelected           = false;
            glm::vec4 layerSelectionBbox = glm::vec4( -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                                      std::numeric_limits<float>::max() );
            for( int j = 0; j < app->layers.data()[i].vertexBuffLength; ++j )
            {
                if( app->viewParams.selectDispatch != mc::SelectDispatch::ComputeBbox )
                {
                    if( app->viewParams.selectDispatch != mc::SelectDispatch::Point &&
                        selectionData[triangleOffset - j].flags != mc::SelectionFlags::InsideBox )
                    {
                        boxSelected = false;
                        break;
                    }

                    if( app->viewParams.selectDispatch == mc::SelectDispatch::Point &&
                        selectionData[triangleOffset - j].flags == mc::SelectionFlags::InsideBox && numSelected == 0 )
                    {
                        pointSelected = true;
                    }
                }

                layerSelectionBbox.x = std::max( layerSelectionBbox.x, selectionData[triangleOffset - j].bbox.x );
                layerSelectionBbox.y = std::max( layerSelectionBbox.y, selectionData[triangleOffset - j].bbox.y );
                layerSelectionBbox.z = std::min( layerSelectionBbox.z, selectionData[triangleOffset - j].bbox.z );
                layerSelectionBbox.w = std::min( layerSelectionBbox.w, selectionData[triangleOffset - j].bbox.w );
            }
            triangleOffset -= app->layers.data()[i].vertexBuffLength;

            // Modify layer selection
            if( ( app->viewParams.selectDispatch != mc::SelectDispatch::ComputeBbox && !boxSelected ) ||
                ( app->viewParams.selectDispatch == mc::SelectDispatch::Point && !pointSelected ) )
            {
                app->layers.changeSelection( i, false );
            }
            else if( app->viewParams.selectDispatch != mc::SelectDispatch::ComputeBbox || app->layers.isSelected( i ) )
            {
                app->selectionAabb.x = std::max( app->selectionAabb.x, layerSelectionBbox.x );
                app->selectionAabb.y = std::max( app->selectionAabb.y, layerSelectionBbox.y );
                app->selectionAabb.z = std::min( app->selectionAabb.z, layerSelectionBbox.z );
                app->selectionAabb.w = std::min( app->selectionAabb.w, layerSelectionBbox.w );

                app->layers.changeSelection( i, true );
                numSelected += 1;
            }
        }
        app->selectionMapBuf.Unmap();

        app->selectionCenter = ( glm::vec2( app->selectionAabb.x, app->selectionAabb.y ) + glm::vec2( app->selectionAabb.z, app->selectionAabb.w ) ) * 0.5f;

        if( app->viewParams.selectDispatch == mc::SelectDispatch::Point || ( app->viewParams.selectDispatch == mc::SelectDispatch::Box && !app->mouseDown ) )
        {
            app->layerHistory.push( app->layers.createShrunkCopy() );
        }

        app->selectionReady            = true;
        app->layersModified            = true;
        app->viewParams.selectDispatch = mc::SelectDispatch::None;
        app->dragType                  = mc::CursorDragType::Select;
    }
    break;
    case mc::Events::AddMergedLayer:
    {
        const mc::Triangle* meshData = reinterpret_cast<const mc::Triangle*>( app->vertexCopyBuf.GetConstMappedRange( 0, app->newMeshSize ) );

        if( meshData == nullptr )
        {
            app->vertexCopyBuf.Unmap();
            return;
        }

        // take the flags and textures from the first mesh in the merge
        uint32_t flags   = app->layers.data()[app->mergeLayerStart].flags;
        uint16_t texture = app->layers.data()[app->mergeLayerStart].texture;
        uint16_t mask    = app->layers.data()[app->mergeLayerStart].mask;
        uint32_t extra0  = app->layers.data()[app->mergeLayerStart].extra0;
        uint32_t extra1  = app->layers.data()[app->mergeLayerStart].extra1;
        uint32_t extra2  = app->layers.data()[app->mergeLayerStart].extra2;
        uint32_t extra3  = app->layers.data()[app->mergeLayerStart].extra3;

        mc::ResourceHandle textureHandle = app->layers.getTexture( app->mergeLayerStart );
        mc::ResourceHandle maskHandle    = app->layers.getMask( app->mergeLayerStart );

        app->layerHistory.resetToCheckpoint();
        app->layers.removeTop( app->mergeLayerStart );
        app->layersModified = true;

        bool ret = app->meshManager.add( meshData, app->newMeshSize / sizeof( mc::Triangle ) );

        app->vertexCopyBuf.Unmap();

        if( !ret )
        {
            return;
        }

        mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( app->meshManager.numMeshes() - 1 );
        app->layers.add( { glm::vec2( 0.0 ), glm::vec2( 1.0, 0.0 ), glm::vec2( 0.0, 1.0 ), glm::u16vec2( 0 ), glm::u16vec2( mc::UV_MAX_VALUE ),
                           glm::u8vec4( 255 ), flags, meshInfo.start, meshInfo.length, texture, mask, extra0, extra1, extra2, extra3 },
                         std::move( textureHandle ), std::move( maskHandle ) );

        app->layers.clearSelection();
        app->layers.changeSelection( app->layers.length() - 1, true );

        app->layerHistory.push( app->layers.createShrunkCopy() );
    }
    break;
    case mc::Events::FlipHorizontal:
        app->layers.scaleSelection( app->selectionCenter, glm::vec2( -1.0, 1.0 ) );
        app->layerHistory.push( app->layers.createShrunkCopy() );
        app->layersModified = true;
        break;
    case mc::Events::FlipVertical:
        app->layers.scaleSelection( app->selectionCenter, glm::vec2( 1.0, -1.0 ) );
        app->layerHistory.push( app->layers.createShrunkCopy() );
        app->layersModified = true;
        break;
    case mc::Events::MoveFront:
        app->layers.bringFrontSelection();
        app->layerHistory.push( app->layers.createShrunkCopy() );
        app->layersModified = true;
        break;
    case mc::Events::MoveBack:
        app->layers.bringFrontSelection( true );
        app->layerHistory.push( app->layers.createShrunkCopy() );
        app->layersModified = true;
        break;
    case mc::Events::Delete:
        app->layers.removeSelection();
        app->layerHistory.push( app->layers.createShrunkCopy() );
        app->layersModified = true;
        break;
    case mc::Events::ChangeMode:
    {
        mc::Mode newMode = eventData->data.mode;
        if( newMode == app->mode )
        {
            return;
        }

        app->mode = newMode;

        if( app->mode == mc::Mode::Cut || app->mode == mc::Mode::SegmentCut )
        {
            int index          = app->layers.getSingleSelectedImage();
            mc::Layer newLayer = app->layers.data()[index];
            newLayer.flags     = newLayer.flags & ~mc::LayerFlags::Selected;

            int width  = app->textureManager.get( app->layers.getTexture( index ) ).texture.GetWidth();
            int height = app->textureManager.get( app->layers.getTexture( index ) ).texture.GetHeight();

            if( app->mode == mc::Mode::SegmentCut && app->mlInference->pipelineValid() )
            {

                for( int i = 0; i < app->textureManager.get( app->layers.getTexture( index ) ).texture.GetMipLevelCount(); ++i )
                {
                    if( width <= app->mlInference->getMaxWidth() && height <= app->mlInference->getMaxHeight() )
                    {
                        break;
                    }

                    width  = width / 2;
                    height = height / 2;
                }

                app->mlInference->resetPoints();

                int index = app->layers.getSingleSelectedImage();

                int mipLevel = std::log2( app->textureManager.get( app->layers.getTexture( index ) ).texture.GetWidth() / width );

                if( app->textureMapBuffer )
                {
                    app->textureMapBuffer.Destroy();
                }

                wgpu::CommandEncoderDescriptor commandEncoderDesc;
                commandEncoderDesc.label     = "get image for SAM";
                wgpu::CommandEncoder encoder = app->device.CreateCommandEncoder( &commandEncoderDesc );

                app->textureMapBuffer =
                    mc::downloadTexture( app->textureManager.get( app->layers.getTexture( index ) ).texture, app->device, encoder, mipLevel );

                wgpu::CommandBuffer command = encoder.Finish();
                app->device.GetQueue().Submit( 1, &command );

                wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userData )
                {
                    if( status == WGPUBufferMapAsyncStatus_Success )
                    {
                        mc::submitEvent( mc::Events::SamLoadInput );
                    }
                };
                app->textureMapBuffer.MapAsync( wgpu::MapMode::Read, 0, app->textureMapBuffer.GetSize(), callback, nullptr );
            }

            app->editMaskTextureHandle = std::make_unique<mc::ResourceHandle>(
                app->textureManager.add( nullptr, width, height, 4, app->device,
                                         wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst ) );
        }

        // save the state at the start of an edit operation
        if( newMode == mc::Mode::Paint || newMode == mc::Mode::Text || newMode == mc::Mode::Crop || newMode == mc::Mode::Cut ||
            newMode == mc::Mode::SegmentCut )
        {
            app->layerHistory.setCheckpoint();
        }

        // todo remove this?
        //  app->layersModified = true;
        mc::changeModeUI( app->mode );
    }
    break;
    case mc::Events::MergeEditLayers:
        app->mergeLayerStart = app->layerHistory.getCheckpoint().length();
        app->mergeTopLayers  = true;
        break;
    case mc::Events::ResetEditLayers:
        app->layers.copyContents( app->layerHistory.resetToCheckpoint() );
        app->layersModified = true;
        break;
    case mc::Events::MergeAndRasterizeRequest:
        app->rasterizeSelection = true;
        break;
    case mc::Events::MergeAndRasterize:
    {
        // find how many layers down to move our new layer
        int selectionTopLayerDelta = 0;
        for( int i = app->layers.length() - 1; i >= 0; --i )
        {
            if( app->layers.isSelected( i ) )
            {
                break;
            }
            selectionTopLayerDelta += 1;
        }
        app->layers.removeSelection();

        int width  = app->selectionAabb.x - app->selectionAabb.z;
        int height = app->selectionAabb.y - app->selectionAabb.w;

        mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( mc::UnitSquareMeshIndex );


        app->layers.add( app->selectionCenter, glm::vec2( width, 0 ), glm::vec2( 0, height ), glm::u16vec2( 0 ), glm::u16vec2( mc::UV_MAX_VALUE ),
                         glm::u8vec4( 255, 255, 255, 255 ), mc::HasColorTex, meshInfo, *app->copyTextureHandle.get() );

        app->layers.changeSelection( app->layers.length() - 1, true );
        app->layers.move( app->layers.length() - selectionTopLayerDelta - 1, app->layers.length() - 1 );

        mc::genMipMaps( app->device, app->mipGenPipeline, app->textureManager.get( *app->copyTextureHandle.get() ).texture );

        app->layersModified = true;
    }
    break;
    case mc::Events::ApplyCrop:
        app->layerHistory.resetToCheckpoint();
        app->layerHistory.push( app->layers.createShrunkCopy() );
        break;
    case mc::Events::ApplyCut:
    {
        app->layers.removeTop( app->layerHistory.getCheckpoint().length() );

        int index = app->layers.getSingleSelectedImage();

        int width  = app->textureManager.get( app->layers.getTexture( index ) ).texture.GetWidth();
        int height = app->textureManager.get( app->layers.getTexture( index ) ).texture.GetHeight();

        mc::ResourceHandle maskedTextureA =
            app->textureManager.add( nullptr, width, height, 4, app->device,
                                     wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc, true );
        mc::ResourceHandle maskedTextureB =
            app->textureManager.add( nullptr, width, height, 4, app->device,
                                     wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc, true );

        const wgpu::BindGroupLayout layout = mc::createWriteTextureBindGroupLayout( app->device );

        wgpu::BindGroup outputBindGroupA = mc::createComputeTextureBindGroup( app->device, app->textureManager.get( maskedTextureA ).texture, layout );
        wgpu::BindGroup outputBindGroupB = mc::createComputeTextureBindGroup( app->device, app->textureManager.get( maskedTextureB ).texture, layout );

        wgpu::CommandEncoderDescriptor commandEncoderDesc;
        commandEncoderDesc.label        = "Cut Mask Encoder";
        wgpu::CommandEncoder cutEncoder = app->device.CreateCommandEncoder( &commandEncoderDesc );

        wgpu::ComputePassEncoder computePassEnc = cutEncoder.BeginComputePass();

        computePassEnc.SetPipeline( app->maskMultiplyPipeline );
        computePassEnc.SetBindGroup( 0, app->textureManager.get( app->layers.getTexture( index ) ).bindGroup );
        computePassEnc.SetBindGroup( 1, app->textureManager.get( *app->editMaskTextureHandle.get() ).bindGroup );
        computePassEnc.SetBindGroup( 2, outputBindGroupA );
        computePassEnc.DispatchWorkgroups( ( width + 8 - 1 ) / 8, ( height + 8 - 1 ) / 8, 1 );

        computePassEnc.SetPipeline( app->invMaskMultiplyPipeline );
        computePassEnc.SetBindGroup( 2, outputBindGroupB );
        computePassEnc.DispatchWorkgroups( ( width + 8 - 1 ) / 8, ( height + 8 - 1 ) / 8, 1 );

        computePassEnc.End();

        wgpu::CommandBuffer command = cutEncoder.Finish();
        app->device.GetQueue().Submit( 1, &command );

        mc::genMipMaps( app->device, app->mipGenPipeline, app->textureManager.get( maskedTextureA ).texture );
        mc::genMipMaps( app->device, app->mipGenPipeline, app->textureManager.get( maskedTextureB ).texture );

        app->layers.add( app->layers.data()[index], maskedTextureA );
        app->layers.add( app->layers.data()[index], maskedTextureB );

        app->layers.remove( index );
        app->layers.move( index, app->layers.length() - 1 );
        app->layers.move( index, app->layers.length() - 1 );
        app->layers.changeSelection( index, false );

        app->layerHistory.resetToCheckpoint();
        app->layerHistory.push( app->layers.createShrunkCopy() );

        app->layersModified = true;
    }
    break;
    case mc::Events::SamLoadInput:
    {
        const uint8_t* imageData = reinterpret_cast<const uint8_t*>( app->textureMapBuffer.GetConstMappedRange( 0, app->textureMapBuffer.GetSize() ) );

        if( imageData == nullptr )
        {
            app->textureMapBuffer.Unmap();
            break;
        }

        int width  = app->textureManager.get( *app->editMaskTextureHandle.get() ).texture.GetWidth();
        int height = app->textureManager.get( *app->editMaskTextureHandle.get() ).texture.GetHeight();

        // we need a width thats a multiple of 256
        size_t textureWidthPadded = ( width + 256 ) / 256 * 256;

        app->mlInference->loadInput( imageData, app->textureMapBuffer.GetSize(), width, height, textureWidthPadded );

        app->textureMapBuffer.Unmap();
    }
    break;
    case mc::Events::SamUploadMask:
        mc::uploadTexture( app->device.GetQueue(), app->textureManager.get( *app->editMaskTextureHandle.get() ).texture, app->mlInference->getMask(),
                           app->mlInference->getInputWidth(), app->mlInference->getInputHeight(), 4 );
        break;
    case mc::Events::AddImageToLayer:
        mc::addImageLayerFromFile( app, std::string( reinterpret_cast<char*>( eventData->sdlUserEvent.data1 ) ) );
        break;
    case mc::Events::Undo:
    {
        const mc::LayerManager& undoLayers = app->layerHistory.undo();
        app->layers.copyContents( undoLayers );
        app->layersModified = true;
        break;
    }
    case mc::Events::Redo:
    {
        const mc::LayerManager& redoLayers = app->layerHistory.redo();
        app->layers.copyContents( redoLayers );
        app->layersModified = true;
        break;
    }
    }

    std::free( eventData->sdlUserEvent.data1 );
}

SDL_AppResult SDL_AppEvent( void* appstate, SDL_Event* event )
{
    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );

    mc::processEventUI( app, event );

    switch( event->type )
    {
    case SDL_EVENT_USER:
        proccessUserEvent( event, app );
        break;
    case SDL_EVENT_QUIT:
        app->appQuit = true;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        app->resetSurface     = true;
        app->resetSurfaceTime = SDL_GetTicks();
        break;
    case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        // do comparison with ints to eliminate precision error
        if( static_cast<int>( app->dpiFactor * 100 ) != static_cast<int>( SDL_GetWindowDisplayScale( app->window ) * 100 ) )
        {
            app->dpiFactor           = SDL_GetWindowDisplayScale( app->window );
            app->viewParams.dpiScale = SDL_GetWindowDisplayScale( app->window );
            app->updateUIStyles      = true;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:

        if( mc::getMouseLocationUI() != mc::MouseLocationUI::Window )
        {
            app->mouseDragStart = app->mouseWindowPos;
            app->mouseDown      = true;
            app->dragType       = mc::CursorDragType::None;

            if( app->mode == mc::Mode::Paint || app->mode == mc::Mode::Cut )
            {
                glm::vec2 basisA      = glm::vec2( 0.0, 2.0 ) * mc::getPaintRadius();
                glm::vec2 basisB      = glm::vec2( 2.0, 0.0 ) * mc::getPaintRadius();
                glm::u8vec4 color     = glm::u8vec4( mc::getPaintColor() * 255.0f, 255 );
                mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( mc::UnitSquareMeshIndex );

                app->layers.add( { app->viewParams.mousePos, basisA, basisB, glm::u16vec2( 0 ), glm::u16vec2( mc::UV_MAX_VALUE ), color, mc::HasPillAlphaTex,
                                   meshInfo.start, meshInfo.length } );
                app->layersModified = true;
            }
            else if( app->mode == mc::Mode::Cursor )
            {
                switch( mc::getMouseLocationUI() )
                {
                case mc::MouseLocationUI::RotateHandle:
                    app->dragType = mc::CursorDragType::Rotate;
                    break;
                case mc::MouseLocationUI::ScaleHandleTL:
                    app->dragType = mc::CursorDragType::ScaleTL;
                    break;
                case mc::MouseLocationUI::ScaleHandleBR:
                    app->dragType = mc::CursorDragType::ScaleBR;
                    break;
                case mc::MouseLocationUI::ScaleHandleTR:
                    app->dragType = mc::CursorDragType::ScaleTR;
                    break;
                case mc::MouseLocationUI::ScaleHandleBL:
                    app->dragType = mc::CursorDragType::ScaleBL;
                    break;
                case mc::MouseLocationUI::MoveHandle:
                    app->dragType = mc::CursorDragType::Move;
                    break;
                default:
                    app->dragType = mc::CursorDragType::Select;
                    break;
                }
            }
            else if( app->mode == mc::Mode::Crop )
            {
                switch( mc::getMouseLocationUI() )
                {
                case mc::MouseLocationUI::ScaleHandleTL:
                    app->dragType = mc::CursorDragType::ScaleTL;
                    break;
                case mc::MouseLocationUI::ScaleHandleBR:
                    app->dragType = mc::CursorDragType::ScaleBR;
                    break;
                case mc::MouseLocationUI::ScaleHandleTR:
                    app->dragType = mc::CursorDragType::ScaleTR;
                    break;
                case mc::MouseLocationUI::ScaleHandleBL:
                    app->dragType = mc::CursorDragType::ScaleBL;
                    break;
                }
            }
            else if( app->mode == mc::Mode::Save )
            {
                app->dragType = mc::CursorDragType::Select;
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:

        if( app->mode == mc::Mode::Save )
        {
            mc::submitEvent( mc::Events::SaveImageRequest );
        }
        else if( app->mode == mc::Mode::SegmentCut && mc::getMouseLocationUI() == mc::MouseLocationUI::MoveHandle && app->mlInference->pipelineValid() &&
                 app->mlInference->inferenceReady() )
        {
            int index       = app->layers.getSingleSelectedImage();
            mc::Layer layer = app->layers.data()[index];

            glm::vec2 scale = static_cast<float>( mc::UV_MAX_VALUE ) / glm::vec2( layer.uvBottom - layer.uvTop );
            layer.basisA *= scale.x;
            layer.basisB *= scale.y;

            glm::vec2 uvCenter = ( glm::vec2( layer.uvTop ) + glm::vec2( layer.uvBottom ) ) / static_cast<float>( mc::UV_MAX_VALUE ) * 0.5f;
            layer.offset -= layer.basisA * ( uvCenter.x - 0.5f ) + layer.basisB * ( uvCenter.y - 0.5f );

            float width  = glm::length( layer.basisA );
            float height = glm::length( layer.basisB );

            glm::vec2 mouseLocal = app->viewParams.mousePos - layer.offset;
            float u              = glm::dot( mouseLocal, layer.basisA ) / glm::dot( layer.basisA, layer.basisA ) + 0.5;
            float v              = glm::dot( mouseLocal, layer.basisB ) / glm::dot( layer.basisB, layer.basisB ) + 0.5;

            app->mlInference->addPoint( glm::vec2( u, v ) );
            app->mlInference->genMask();
            submitEvent( mc::Events::SamUploadMask );
        }
        else if( app->dragType != mc::CursorDragType::Select && app->mode != mc::Mode::Pan )
        {
            app->layerHistory.push( app->layers.createShrunkCopy() );
        }

        // click selection if the mouse hasnt moved since mouse down
        if( app->mouseWindowPos == app->mouseDragStart && app->mode == mc::Mode::Cursor )
        {
            app->viewParams.selectDispatch = mc::SelectDispatch::Point;
        }
        else if( app->dragType != mc::CursorDragType::Select )
        {
            // something was transformed so recalculate bbox
            app->viewParams.selectDispatch = mc::SelectDispatch::ComputeBbox;
        }
        app->mouseDown = false;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app->mouseWindowPos = glm::vec2( event->motion.x, event->motion.y );
        app->mouseDelta += glm::vec2( event->motion.xrel, event->motion.yrel );

        if( app->mouseDown && mc::getMouseLocationUI() != mc::MouseLocationUI::Window && app->mode == mc::Mode::Pan )
        {
            app->updateView = true;
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        app->scrollDelta += glm::vec2( event->wheel.x, event->wheel.y );

        if( mc::getMouseLocationUI() != mc::MouseLocationUI::Window )
        {
            app->updateView = true;
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        break;
    case SDL_EVENT_KEY_UP:
        break;
    default:
        break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate( void* appstate )
{
    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->instance.ProcessEvents();
#endif

    if( app->resetSurface && SDL_GetTicks() - app->resetSurfaceTime > mc::resetSurfaceDelayMs )
    {
        SDL_GetWindowSize( app->window, &app->width, &app->height );
        SDL_GetWindowSizeInPixels( app->window, &app->bbwidth, &app->bbheight );
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        mc::configureSurface( app );
#endif
        app->canvasRenderTextureHandle      = std::make_unique<mc::ResourceHandle>( app->textureManager.add(
            nullptr, app->bbwidth, app->bbheight, 4, app->device, wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding ) );
        app->canvasSelectMaskHandle         = std::make_unique<mc::ResourceHandle>( app->textureManager.add(
            nullptr, app->bbwidth, app->bbheight, 1, app->device, wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding ) );
        app->canvasSelectOccludedMaskHandle = std::make_unique<mc::ResourceHandle>( app->textureManager.add(
            nullptr, app->bbwidth, app->bbheight, 1, app->device, wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding ) );

        app->resetSurface = false;
        app->updateView   = true;
    }

    if( app->resetSurface )
    {
        return SDL_APP_CONTINUE;
    }

    if( app->updateUIStyles )
    {
        mc::setStylesUI( app );
        app->updateUIStyles = false;
    }

    // Update canvas offset
    if( app->mouseDelta.length() > 0.0 && app->mouseDown && app->updateView && app->mode == mc::Mode::Pan )
    {
        app->viewParams.canvasPos += app->mouseDelta;
    }

    // Update mouse position in canvas coordinate space
    app->viewParams.mousePos       = ( app->mouseWindowPos - app->viewParams.canvasPos ) / app->viewParams.scale;
    app->viewParams.mouseSelectPos = ( app->mouseDragStart - app->viewParams.canvasPos ) / app->viewParams.scale;

    // Update zoom level
    // For zoom, we want to center it around the mouse position
    if( app->scrollDelta.y != 0.0 && app->updateView )
    {
        float zoomFactor = std::exp( app->scrollDelta.y * mc::ZoomScaleFactor );
        float newScale   = std::max<float>( std::numeric_limits<float>::min(), app->viewParams.scale * zoomFactor );

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

        app->updateView = false;
    }

    if( app->mode == mc::Mode::Cursor && app->mouseDown && app->dragType != mc::CursorDragType::None && app->mouseDelta != glm::vec2( 0.0 ) )
    {
        switch( app->dragType )
        {
        case mc::CursorDragType::Select:
        {
            app->viewParams.selectDispatch = mc::SelectDispatch::Box;
        }
        break;
        case mc::CursorDragType::Move:
        {
            app->layers.translateSelection( app->mouseDelta / app->viewParams.scale );
            app->layersModified = true;
        }
        break;
        case mc::CursorDragType::Rotate:
        {
            glm::vec2 p1       = app->viewParams.mousePos - app->selectionCenter;
            glm::vec2 p2       = p1 - app->mouseDelta / app->viewParams.scale;
            float angle        = acos( std::clamp( glm::dot( p1, p2 ) / ( glm::length( p1 ) * glm::length( p2 ) ), -1.0f, 1.0f ) );
            float partialCross = p1.x * p2.y - p2.x * p1.y;

            app->layers.rotateSelection( app->selectionCenter, partialCross < 0.0 ? angle : -angle );
            app->layersModified = true;
        }
        break;
        case mc::CursorDragType::ScaleTR:
        case mc::CursorDragType::ScaleBR:
        case mc::CursorDragType::ScaleTL:
        case mc::CursorDragType::ScaleBL:
        {
            glm::vec2 p1 = app->viewParams.mousePos - app->selectionCenter;
            glm::vec2 p2 = p1 - app->mouseDelta / app->viewParams.scale;

            float factor = glm::length( p1 ) / glm::length( p2 );

            glm::vec2 orientation = glm::vec2( 1.0 );
            if( p1.x * p2.x < 0 )
            {
                orientation.x = -1.0;
            }
            if( p1.y * p2.y < 0 )
            {
                orientation.y = -1.0;
            }

            app->layers.scaleSelection( app->selectionCenter, glm::vec2( factor ) * orientation );
            app->layersModified = true;
        }
        break;
        }
    }

    if( app->mode == mc::Mode::Crop && app->mouseDown && app->dragType != mc::CursorDragType::None && app->mouseDelta != glm::vec2( 0.0 ) )
    {
        // this might be easier to do if we construct the layer transform and inverse transform matricies but whatever

        int index       = app->layers.getSingleSelectedImage();
        mc::Layer layer = app->layers.data()[index];

        // check if the layer is mirrored
        glm::vec3 cross = glm::cross( glm::vec3( layer.basisA, 0.0f ), glm::vec3( layer.basisB, 0.0f ) );
        bool mirrored   = glm::sign( cross ).z < 0.0f;

        // set up all necessary data and calculate the "uncropped" layer data
        glm::vec2 basisA   = glm::normalize( layer.basisA );
        glm::vec2 basisB   = glm::normalize( layer.basisB );
        float width        = glm::length( layer.basisA );
        float height       = glm::length( layer.basisB );
        float offsetLocalx = glm::dot( layer.offset, basisA ) / glm::dot( basisA, basisA );
        float offsetLocaly = glm::dot( layer.offset, basisB ) / glm::dot( basisB, basisB );

        mc::Layer uncroppedLayer = app->layers.getUncroppedLayer( index );

        float uncroppedWidth        = glm::length( uncroppedLayer.basisA );
        float uncroppedHeight       = glm::length( uncroppedLayer.basisB );
        float uncroppedOffsetLocalx = glm::dot( uncroppedLayer.offset, basisA ) / glm::dot( basisA, basisA );
        float uncroppedOffsetLocaly = glm::dot( uncroppedLayer.offset, basisB ) / glm::dot( basisA, basisA );

        // determine scaling factors along the basis vectors
        glm::vec2 mouseDeltaCanvas = app->mouseDelta / app->viewParams.scale;
        float mouseDeltaLocalx     = glm::dot( mouseDeltaCanvas, basisA ) / glm::dot( basisA, basisA );
        float mouseDeltaLocaly     = glm::dot( mouseDeltaCanvas, basisB ) / glm::dot( basisB, basisB );

        glm::vec2 orientation = glm::vec2( 1.0f );
        if( app->dragType == mc::CursorDragType::ScaleTR )
        {
            orientation.y = -1.0;
        }
        else if( app->dragType == mc::CursorDragType::ScaleTL )
        {
            orientation = -orientation;
        }
        else if( app->dragType == mc::CursorDragType::ScaleBL )
        {
            orientation.x = -1.0;
        }

        if( mirrored )
        {
            orientation.x *= -1.0;
        }

        glm::vec2 uncroppedCornerTop    = glm::vec2( uncroppedOffsetLocalx, uncroppedOffsetLocaly ) - 0.5f * glm::vec2( uncroppedWidth, uncroppedHeight );
        glm::vec2 uncroppedCornerBottom = glm::vec2( uncroppedOffsetLocalx, uncroppedOffsetLocaly ) + 0.5f * glm::vec2( uncroppedWidth, uncroppedHeight );

        glm::vec2 croppedCornerTop = glm::vec2( offsetLocalx + mouseDeltaLocalx * 0.5f, offsetLocaly + mouseDeltaLocaly * 0.5f ) -
                                     0.5f * glm::vec2( width + orientation.x * mouseDeltaLocalx, height + orientation.y * mouseDeltaLocaly );
        glm::vec2 croppedCornerBottom = glm::vec2( offsetLocalx + mouseDeltaLocalx * 0.5f, offsetLocaly + mouseDeltaLocaly * 0.5f ) +
                                        0.5f * glm::vec2( width + orientation.x * mouseDeltaLocalx, height + orientation.y * mouseDeltaLocaly );

        // clamp scaling to not exceed uncropped image extends
        glm::vec2 newCroppedCornerTop    = glm::max( croppedCornerTop, uncroppedCornerTop );
        glm::vec2 newCroppedCornerBottom = glm::min( croppedCornerBottom, uncroppedCornerBottom );

        if( newCroppedCornerBottom.x - newCroppedCornerTop.x < 1.0f )
        {
            if( app->dragType == mc::CursorDragType::ScaleTR || app->dragType == mc::CursorDragType::ScaleBR )
            {
                newCroppedCornerBottom.x = newCroppedCornerTop.x + 1.0f;
            }
            else
            {
                newCroppedCornerTop.x = newCroppedCornerBottom.x - 1.0f;
            }
        }
        if( newCroppedCornerBottom.y - newCroppedCornerTop.y < 1.0f )
        {
            if( app->dragType == mc::CursorDragType::ScaleBR || app->dragType == mc::CursorDragType::ScaleBL )
            {
                newCroppedCornerBottom.y = newCroppedCornerTop.y + 1.0f;
            }
            else
            {
                newCroppedCornerTop.y = newCroppedCornerBottom.y - 1.0f;
            }
        }

        croppedCornerTop    = newCroppedCornerTop;
        croppedCornerBottom = newCroppedCornerBottom;

        app->layers.data()[index].uvTop =
            ( croppedCornerTop - uncroppedCornerTop ) / ( uncroppedCornerBottom - uncroppedCornerTop ) * static_cast<float>( mc::UV_MAX_VALUE );
        app->layers.data()[index].uvBottom =
            ( croppedCornerBottom - uncroppedCornerTop ) / ( uncroppedCornerBottom - uncroppedCornerTop ) * static_cast<float>( mc::UV_MAX_VALUE );

        app->layers.data()[index].basisA = basisA * std::abs( croppedCornerBottom.x - croppedCornerTop.x );
        app->layers.data()[index].basisB = basisB * std::abs( croppedCornerBottom.y - croppedCornerTop.y );

        glm::vec2 localCenter            = ( croppedCornerTop + croppedCornerBottom ) * 0.5f;
        app->layers.data()[index].offset = basisA * localCenter.x + basisB * localCenter.y;

        app->layersModified = true;
    }


    if( ( app->mode == mc::Mode::Paint || app->mode == mc::Mode::Cut ) && app->mouseDown && app->mouseDelta != glm::vec2( 0.0 ) )
    {
        glm::vec2 basisA      = app->mouseDelta / app->viewParams.scale + 2.0f * glm::normalize( app->mouseDelta ) * mc::getPaintRadius();
        glm::vec2 basisB      = 2.0f * glm::normalize( glm::vec2( app->mouseDelta.y, -app->mouseDelta.x ) ) * mc::getPaintRadius();
        glm::u8vec4 color     = glm::u8vec4( mc::getPaintColor() * 255.0f, 255 );
        mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( mc::UnitSquareMeshIndex );

        app->layers.add( { app->viewParams.mousePos - app->mouseDelta / app->viewParams.scale * 0.5f, basisA, basisB, glm::u16vec2( 0 ),
                           glm::u16vec2( mc::UV_MAX_VALUE ), color, mc::HasPillAlphaTex, meshInfo.start, meshInfo.length } );
        app->layersModified = true;
    }
    else if( app->mode == mc::Mode::Text && !app->mergeTopLayers )
    {
        app->layers.removeTop( app->layerHistory.getCheckpoint().length() );

        app->fontManager.buildText( mc::getInputTextString(), mc::getInputTextFont(), app->layers, mc::getInputTextAlignment(),
                                    ( glm::vec2( app->width, app->height ) * 0.5f - app->viewParams.canvasPos ) / app->viewParams.scale,
                                    1.0 / app->viewParams.scale, mc::getInputTextColor(), mc::getInputTextOutline(), mc::getInputTextOutlineColor() );
        app->layersModified = true;
    }

    if( app->layersModified )
    {
        app->viewParams.numLayers = static_cast<uint32_t>( app->layers.length() );
    }

    app->viewParams.ticks = static_cast<uint32_t>( SDL_GetTicks() );
    app->device.GetQueue().WriteBuffer( app->viewParamBuf, 0, &app->viewParams, sizeof( mc::Uniforms ) );

    wgpu::CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Casper";

    wgpu::CommandEncoder encoder = app->device.CreateCommandEncoder( &commandEncoderDesc );

    if( app->layersModified )
    {
        app->device.GetQueue().WriteBuffer( app->layerBuf, 0, app->layers.data(), app->layers.length() * sizeof( mc::Layer ) );

        updateMeshBuffers( app );

        wgpu::ComputePassEncoder computePassEnc = encoder.BeginComputePass();
        computePassEnc.SetPipeline( app->meshPipeline );
        computePassEnc.SetBindGroup( 0, app->globalBindGroup );
        computePassEnc.SetBindGroup( 1, app->meshBindGroup );

        computePassEnc.DispatchWorkgroups( ( app->layers.getTotalTriCount() + 256 - 1 ) / 256, 1, 1 );
        computePassEnc.End();

        app->layersModified = false;
    }

    if( app->mergeTopLayers )
    {
        int newMeshOffset = 0;
        app->newMeshSize  = 0;
        for( int i = 0; i < app->layers.length(); ++i )
        {
            if( i < app->mergeLayerStart )
            {
                newMeshOffset += app->layers.data()[i].vertexBuffLength * sizeof( mc::Triangle );
            }
            else
            {
                app->newMeshSize += app->layers.data()[i].vertexBuffLength * sizeof( mc::Triangle );
            }
        }
        if( app->newMeshSize + app->meshManager.size() > mc::MaxMeshBufferSize )
        {
            // cant merge because our mesh manager buffer will overflow
            submitEvent( mc::Events::ResetEditLayers );
            app->mergeTopLayers = false;
        }
        else
        {
            encoder.CopyBufferToBuffer( app->vertexBuf, newMeshOffset, app->vertexCopyBuf, 0, app->newMeshSize );
        }
    }

    wgpu::RenderPassEncoder canvasRenderPassEnc =
        mc::createRenderPassEncoder<3>( encoder,
                                        { app->textureManager.get( *app->canvasRenderTextureHandle.get() ).textureView,
                                          app->textureManager.get( *app->canvasSelectMaskHandle.get() ).textureView,
                                          app->textureManager.get( *app->canvasSelectOccludedMaskHandle.get() ).textureView },
                                        { wgpu::Color{ Spectrum::ColorR( Spectrum::Static::BONE ), Spectrum::ColorG( Spectrum::Static::BONE ),
                                                       Spectrum::ColorB( Spectrum::Static::BONE ), 1.0f },
                                          wgpu::Color{ 0.0, 0.0, 0.0, 1.0f }, wgpu::Color{ 0.0, 0.0, 0.0, 1.0f } } );

    if( app->layers.length() > 0 )
    {
        canvasRenderPassEnc.SetPipeline( app->canvasPipeline );
        canvasRenderPassEnc.SetVertexBuffer( 0, app->vertexBuf );
        canvasRenderPassEnc.SetVertexBuffer( 1, app->layerBuf );
        canvasRenderPassEnc.SetBindGroup( 0, app->globalBindGroup );

        // webgpu doesnt have texture arrays or bindless textures so we cant use batch rendering
        // for now draw each layer with a seperate command
        int offset = 0;
        for( int i = 0; i < app->layers.length(); ++i )
        {
            if( app->mode == mc::Mode::Cut && i == app->layerHistory.getCheckpoint().length() )
            {
                break;
            }

            app->textureManager.bind( app->layers.getTexture( i ), 1, canvasRenderPassEnc );
            app->textureManager.bind( app->layers.getMask( i ), 2, canvasRenderPassEnc );
            canvasRenderPassEnc.Draw( app->layers.data()[i].vertexBuffLength * 3, 1, offset );
            offset += app->layers.data()[i].vertexBuffLength * 3;
        }
    }

    canvasRenderPassEnc.End();

    wgpu::SurfaceTexture surfaceTexture;
    app->surface.GetCurrentTexture( &surfaceTexture );

    wgpu::RenderPassEncoder postRenderPassEnc =
        mc::createRenderPassEncoder<1>( encoder, { surfaceTexture.texture.CreateView() },
                                        { wgpu::Color{ Spectrum::ColorR( Spectrum::Static::BONE ), Spectrum::ColorG( Spectrum::Static::BONE ),
                                                       Spectrum::ColorB( Spectrum::Static::BONE ), 1.0f } } );

    postRenderPassEnc.SetPipeline( app->postPipeline );

    postRenderPassEnc.SetBindGroup( 0, app->globalBindGroup );
    app->textureManager.bind( *app->canvasRenderTextureHandle.get(), 1, postRenderPassEnc );
    app->textureManager.bind( *app->canvasSelectMaskHandle.get(), 2, postRenderPassEnc );
    app->textureManager.bind( *app->canvasSelectOccludedMaskHandle.get(), 3, postRenderPassEnc );

    postRenderPassEnc.Draw( 6 );

    mc::drawUI( app, postRenderPassEnc );

    postRenderPassEnc.End();

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label   = "Melchior";
    wgpu::CommandBuffer command = encoder.Finish( &cmdBufferDescriptor );

    app->device.GetQueue().Submit( 1, &command );

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->surface.Present();
#endif

    commandEncoderDesc.label              = "Secondary Encoder";
    wgpu::CommandEncoder secondaryEncoder = app->device.CreateCommandEncoder( &commandEncoderDesc );

    if( app->saveImage )
    {
        int saveMinX = std::min( app->mouseDragStart.x, app->mouseWindowPos.x );
        int saveMinY = std::min( app->mouseDragStart.y, app->mouseWindowPos.y );
        int saveMaxX = std::max( app->mouseDragStart.x, app->mouseWindowPos.x );
        int saveMaxY = std::max( app->mouseDragStart.y, app->mouseWindowPos.y );

        saveMinX = std::clamp( saveMinX, 0, app->width );
        saveMinY = std::clamp( saveMinY, 0, app->height );
        saveMaxX = std::clamp( saveMaxX, 0, app->width );
        saveMaxY = std::clamp( saveMaxY, 0, app->height );

        int saveWidth  = saveMaxX - saveMinX;
        int saveHeight = saveMaxY - saveMinY;

        app->copyTextureHandle = std::make_unique<mc::ResourceHandle>(
            app->textureManager.add( nullptr, saveWidth, saveHeight, 4, app->device,
                                     wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding ) );

        if( app->copyTextureHandle->valid() && saveWidth > 0 && saveHeight > 0 )
        {
            mc::Uniforms outputViewParams = app->viewParams;
            outputViewParams.viewType     = mc::ViewType::CaptureTarget;

            float l = ( saveMinX - app->viewParams.canvasPos.x ) * 1.0 / app->viewParams.scale;
            float r = ( saveWidth + saveMinX - app->viewParams.canvasPos.x ) * 1.0 / app->viewParams.scale;
            float t = ( saveMinY - app->viewParams.canvasPos.y ) * 1.0 / app->viewParams.scale;
            float b = ( saveHeight + saveMinY - app->viewParams.canvasPos.y ) * 1.0 / app->viewParams.scale;

            outputViewParams.proj = glm::mat4( 2.0 / ( r - l ), 0.0, 0.0, ( r + l ) / ( l - r ), 0.0, 2.0 / ( t - b ), 0.0, ( t + b ) / ( b - t ), 0.0, 0.0,
                                               0.5, 0.5, 0.0, 0.0, 0.0, 1.0 );

            app->device.GetQueue().WriteBuffer( app->viewParamBuf, 0, &outputViewParams, sizeof( mc::Uniforms ) );

            wgpu::Color backgroundColor = mc::getSaveWithTransparency()
                                              ? wgpu::Color{ 0.0, 0.0, 0.0, 0.0f }
                                              : wgpu::Color{ Spectrum::ColorR( Spectrum::Static::BONE ), Spectrum::ColorG( Spectrum::Static::BONE ),
                                                             Spectrum::ColorB( Spectrum::Static::BONE ), 1.0f };

            wgpu::RenderPassEncoder outputRenderPassEnc = mc::createRenderPassEncoder<1>(
                secondaryEncoder, { app->textureManager.get( *app->copyTextureHandle.get() ).textureView }, { backgroundColor } );

            if( app->layers.length() > 0 )
            {
                outputRenderPassEnc.SetPipeline( app->exportPipeline );
                outputRenderPassEnc.SetVertexBuffer( 0, app->vertexBuf );
                outputRenderPassEnc.SetVertexBuffer( 1, app->layerBuf );
                outputRenderPassEnc.SetBindGroup( 0, app->globalBindGroup );

                int offset = 0;
                for( int i = 0; i < app->layers.length(); ++i )
                {
                    app->textureManager.bind( app->layers.getTexture( i ), 1, outputRenderPassEnc );
                    app->textureManager.bind( app->layers.getMask( i ), 2, outputRenderPassEnc );
                    outputRenderPassEnc.Draw( app->layers.data()[i].vertexBuffLength * 3, 1, offset );
                    offset += app->layers.data()[i].vertexBuffLength * 3;
                }
            }

            outputRenderPassEnc.End();

            if( app->textureMapBuffer )
            {
                app->textureMapBuffer.Destroy();
            }

            app->textureMapBuffer = mc::downloadTexture( app->textureManager.get( *app->copyTextureHandle.get() ).texture, app->device, secondaryEncoder );

            mc::submitEvent( mc::Events::ChangeMode, { .mode = mc::Mode::Cursor } );
        }
    }

    if( app->rasterizeSelection )
    {
        int rasterWidth  = ( app->selectionAabb.x - app->selectionAabb.z ) * app->viewParams.scale;
        int rasterHeight = ( app->selectionAabb.y - app->selectionAabb.w ) * app->viewParams.scale;

        app->copyTextureHandle = std::make_unique<mc::ResourceHandle>( app->textureManager.add(
            nullptr, rasterWidth, rasterHeight, 4, app->device,
            wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding,
            true ) );

        if( app->copyTextureHandle->valid() && rasterWidth > 0 && rasterHeight > 0 )
        {
            mc::Uniforms outputViewParams = app->viewParams;
            outputViewParams.viewType     = mc::ViewType::SelectionRasterTarget;

            float l = app->selectionAabb.z;
            float r = app->selectionAabb.x;
            float t = app->selectionAabb.w;
            float b = app->selectionAabb.y;

            outputViewParams.proj = glm::mat4( 2.0 / ( r - l ), 0.0, 0.0, ( r + l ) / ( l - r ), 0.0, 2.0 / ( t - b ), 0.0, ( t + b ) / ( b - t ), 0.0, 0.0,
                                               0.5, 0.5, 0.0, 0.0, 0.0, 1.0 );

            app->device.GetQueue().WriteBuffer( app->viewParamBuf, 0, &outputViewParams, sizeof( mc::Uniforms ) );

            wgpu::TextureViewDescriptor textureViewDesc;
            textureViewDesc.aspect          = wgpu::TextureAspect::All;
            textureViewDesc.baseArrayLayer  = 0;
            textureViewDesc.arrayLayerCount = 1;
            textureViewDesc.baseMipLevel    = 0;
            textureViewDesc.mipLevelCount   = 1;
            textureViewDesc.dimension       = wgpu::TextureViewDimension::e2D;
            textureViewDesc.format          = app->textureManager.get( *app->copyTextureHandle.get() ).texture.GetFormat();

            wgpu::TextureView textureView = app->textureManager.get( *app->copyTextureHandle.get() ).texture.CreateView( &textureViewDesc );

            wgpu::RenderPassEncoder outputRenderPassEnc =
                mc::createRenderPassEncoder<1>( secondaryEncoder, { textureView }, { wgpu::Color{ 0.0, 0.0, 0.0, 0.0f } } );

            if( app->layers.length() > 0 )
            {
                outputRenderPassEnc.SetPipeline( app->exportPipeline );
                outputRenderPassEnc.SetVertexBuffer( 0, app->vertexBuf );
                outputRenderPassEnc.SetVertexBuffer( 1, app->layerBuf );
                outputRenderPassEnc.SetBindGroup( 0, app->globalBindGroup );

                int offset = 0;
                for( int i = 0; i < app->layers.length(); ++i )
                {
                    app->textureManager.bind( app->layers.getTexture( i ), 1, outputRenderPassEnc );
                    app->textureManager.bind( app->layers.getMask( i ), 2, outputRenderPassEnc );
                    outputRenderPassEnc.Draw( app->layers.data()[i].vertexBuffLength * 3, 1, offset );
                    offset += app->layers.data()[i].vertexBuffLength * 3;
                }
            }

            outputRenderPassEnc.End();

            submitEvent( mc::Events::MergeAndRasterize );
        }
        app->rasterizeSelection = false;
    }

    if( ( app->mode == mc::Mode::Cut ) && app->layers.length() > 0 )
    {
        int index       = app->layers.getSingleSelectedImage();
        mc::Layer layer = app->layers.data()[index];

        glm::vec2 scale = static_cast<float>( mc::UV_MAX_VALUE ) / glm::vec2( layer.uvBottom - layer.uvTop );
        layer.basisA *= scale.x;
        layer.basisB *= scale.y;

        glm::vec2 uvCenter = ( glm::vec2( layer.uvTop ) + glm::vec2( layer.uvBottom ) ) / static_cast<float>( mc::UV_MAX_VALUE ) * 0.5f;
        layer.offset -= layer.basisA * ( uvCenter.x - 0.5f ) + layer.basisB * ( uvCenter.y - 0.5f );

        layer.basisA *= 0.5f;
        layer.basisB *= -0.5f;

        float det           = layer.basisA.x * layer.basisB.y - layer.basisA.y * layer.basisB.x;
        glm::vec2 invBasisA = glm::vec2( layer.basisB.y, -layer.basisA.y ) / det;
        glm::vec2 invBasisB = glm::vec2( -layer.basisB.x, layer.basisA.x ) / det;
        glm::vec2 invOffset =
            -glm::vec2( layer.offset.x * invBasisA.x + layer.offset.y * invBasisB.x, layer.offset.x * invBasisA.y + layer.offset.y * invBasisB.y );

        mc::Uniforms maskUniforms = app->viewParams;
        maskUniforms.proj = glm::mat4x4( invBasisA.x, invBasisA.y, 0.0f, invOffset.x, invBasisB.x, invBasisB.y, 0.0f, invOffset.y, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                         0.0f, 0.0f, 1.0f );

        app->device.GetQueue().WriteBuffer( app->viewParamBuf, 0, &maskUniforms, sizeof( mc::Uniforms ) );

        wgpu::CommandEncoderDescriptor commandEncoderDesc;
        commandEncoderDesc.label = "Mask Render";

        wgpu::RenderPassEncoder maskRenderPassEnc = mc::createRenderPassEncoder<1>(
            secondaryEncoder, { app->textureManager.get( *app->editMaskTextureHandle.get() ).textureView }, { wgpu::Color{ 1.0f, 1.0f, 1.0f, 1.0f } } );

        maskRenderPassEnc.SetPipeline( app->exportPipeline );
        maskRenderPassEnc.SetVertexBuffer( 0, app->vertexBuf );
        maskRenderPassEnc.SetVertexBuffer( 1, app->layerBuf );
        maskRenderPassEnc.SetBindGroup( 0, app->globalBindGroup );

        app->textureManager.bind( mc::ResourceHandle::invalidResource(), 1, maskRenderPassEnc );
        app->textureManager.bind( mc::ResourceHandle::invalidResource(), 2, maskRenderPassEnc );

        int firstTriangle = 0;
        for( int i = 0; i < app->layerHistory.getCheckpoint().length(); ++i )
        {
            firstTriangle += app->layers.data()[i].vertexBuffLength;
        }
        maskRenderPassEnc.Draw( ( app->layers.getTotalTriCount() - firstTriangle ) * 3, 1, firstTriangle * 3 );

        maskRenderPassEnc.End();
    }

    // We only want to recompute the selection array if a selection is requested and any
    // previously requested computations have completed aka selection ready
    bool computeSelection = app->viewParams.selectDispatch != mc::SelectDispatch::None && app->selectionReady && app->layers.length() > 0;
    if( computeSelection )
    {
        secondaryEncoder.ClearBuffer( app->selectionBuf, 0, app->layers.length() * sizeof( mc::Selection ) );
        wgpu::ComputePassEncoder computePassEnc = secondaryEncoder.BeginComputePass();
        computePassEnc.SetPipeline( app->selectionPipeline );
        computePassEnc.SetBindGroup( 0, app->globalBindGroup );
        computePassEnc.SetBindGroup( 1, app->meshBindGroup );
        computePassEnc.SetBindGroup( 2, app->selectionBindGroup );

        computePassEnc.DispatchWorkgroups( ( app->layers.getTotalTriCount() + 256 - 1 ) / 256, 1, 1 );
        computePassEnc.End();

        secondaryEncoder.CopyBufferToBuffer( app->selectionBuf, 0, app->selectionMapBuf, 0, app->layers.getTotalTriCount() * sizeof( mc::Selection ) );
    }

    cmdBufferDescriptor.label             = "Secondary Command Buffer";
    wgpu::CommandBuffer secondaryCommands = secondaryEncoder.Finish( &cmdBufferDescriptor );

    app->device.GetQueue().Submit( 1, &secondaryCommands );

    // Reset
    app->mouseDelta  = glm::vec2( 0.0 );
    app->scrollDelta = glm::vec2( 0.0 );

    if( app->mergeTopLayers )
    {
        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userData )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                submitEvent( mc::Events::AddMergedLayer );
            }
            else
            {
                submitEvent( mc::Events::ResetEditLayers );
            }
        };
        app->vertexCopyBuf.MapAsync( wgpu::MapMode::Read, 0, app->newMeshSize, callback, nullptr );

        app->mergeTopLayers = false;
    }

    if( computeSelection )
    {
        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userData )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                submitEvent( mc::Events::SelectionChanged );
            }
        };
        app->selectionMapBuf.MapAsync( wgpu::MapMode::Read, 0, app->layers.getTotalTriCount() * sizeof( mc::Selection ), callback, nullptr );

        app->selectionReady = false;
    }

    if( app->saveImage && app->textureMapBuffer.GetMapState() == wgpu::BufferMapState::Unmapped )
    {
        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userData )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                submitEvent( mc::Events::SaveImage );
            }
        };
        app->textureMapBuffer.MapAsync( wgpu::MapMode::Read, 0, app->textureMapBuffer.GetSize(), callback, nullptr );

        app->saveImage = false;
    }

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->device.Tick();
#endif

    return app->appQuit ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

void SDL_AppQuit( void* appstate, SDL_AppResult result )
{
    mc::shutdownUI();

    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );
    if( app )
    {
        SDL_DestroyWindow( app->window );
    }

    SDL_Quit();
    SDL_Log( "Application quit successfully!" );
}
