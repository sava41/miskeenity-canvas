#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
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
#include "layer_mesh_utils.h"
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

    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMEPAD ) )
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

    mc::initDevice( app );

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
    app->dpiFactor = SDL_GetWindowDisplayScale( app->window );
    mc::configureSurface( app );

    mc::initUI( app );

    mc::initMainPipeline( app );

    // print some information about the window
    SDL_ShowWindow( app->window );
    {
        SDL_Log( "Window size: %ix%i", app->width, app->height );
        SDL_Log( "Backbuffer size: %ix%i", app->bbwidth, app->bbheight );
    }

    app->textureManager.init( app->device );

    app->updateView = true;

    SDL_Log( "Application started successfully!" );

    return SDL_APP_CONTINUE;
}

void proccessUserEvent( const SDL_Event* event, mc::AppContext* app )
{
    switch( static_cast<mc::Events>( event->user.code ) )
    {
    case mc::Events::AppQuit:
        app->appQuit = true;
        break;
    case mc::Events::LoadImage:
        mc::loadImageFromFileDialog( app );
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
            app->selectionReady = true;
            return;
        }

        app->selectionBbox = glm::vec4( -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
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
                app->selectionBbox.x = std::max( app->selectionBbox.x, layerSelectionBbox.x );
                app->selectionBbox.y = std::max( app->selectionBbox.y, layerSelectionBbox.y );
                app->selectionBbox.z = std::min( app->selectionBbox.z, layerSelectionBbox.z );
                app->selectionBbox.w = std::min( app->selectionBbox.w, layerSelectionBbox.w );

                app->layers.changeSelection( i, true );
                numSelected += 1;
            }
        }
        app->selectionMapBuf.Unmap();

        app->selectionCenter = ( glm::vec2( app->selectionBbox.x, app->selectionBbox.y ) + glm::vec2( app->selectionBbox.z, app->selectionBbox.w ) ) * 0.5f;

        app->selectionReady            = true;
        app->layersModified            = true;
        app->viewParams.selectDispatch = mc::SelectDispatch::None;
        app->dragType                  = mc::CursorDragType::Select;
    }
    break;
    case mc::Events::MergeTopLayers:
    {
        const mc::Triangle* meshData = reinterpret_cast<const mc::Triangle*>( app->vertexCopyBuf.GetConstMappedRange( 0, app->newMeshSize ) );

        if( meshData == nullptr )
        {
            app->vertexCopyBuf.Unmap();
            return;
        }

        app->layers.removeTop( app->layerEditStart );
        app->layersModified = true;

        bool ret = app->meshManager.add( meshData, app->newMeshSize / sizeof( mc::Triangle ) );

        app->vertexCopyBuf.Unmap();

        if( !ret )
        {
            return;
        }

        mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( app->meshManager.numMeshes() - 1 );
        app->layers.add( { glm::vec2( 0.0 ), glm::vec2( 1.0, 0.0 ), glm::vec2( 0.0, 1.0 ), glm::u16vec2( 0 ), glm::u16vec2( 1.0 ), glm::u8vec4( 255 ),
                           mc::HasPillAlphaTex, meshInfo.start, meshInfo.length, 0, 0 } );
    }
    break;
    case mc::Events::FlipHorizontal:
        app->layers.scaleSelection( app->selectionCenter, glm::vec2( -1.0, 1.0 ) );
        app->layersModified = true;
        break;
    case mc::Events::FlipVertical:
        app->layers.scaleSelection( app->selectionCenter, glm::vec2( 1.0, -1.0 ) );
        app->layersModified = true;
        break;
    case mc::Events::MoveFront:
        app->layers.bringFrontSelection();
        app->layersModified = true;
        break;
    case mc::Events::MoveBack:
        app->layers.bringFrontSelection( true );
        app->layersModified = true;
        break;
    case mc::Events::Delete:
        app->layers.removeSelection();
        app->layersModified = true;
        break;
    case mc::Events::ModeChanged:
        app->layerEditStart = app->layers.length();
        break;
    case mc::Events::ContextAccept:
        app->mergeTopLayers = true;
        break;
    case mc::Events::ContextCancel:
        app->layers.removeTop( app->layerEditStart );
        app->layersModified = true;
        break;
    }
}

SDL_AppResult SDL_AppEvent( void* appstate, const SDL_Event* event )
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
            app->dpiFactor      = SDL_GetWindowDisplayScale( app->window );
            app->updateUIStyles = true;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:

        if( mc::getMouseLocationUI() != mc::MouseLocationUI::Window )
        {
            app->mouseDragStart = app->mouseWindowPos;
            app->mouseDown      = true;

            if( mc::getAppMode() == mc::Mode::Paint )
            {
                glm::vec2 basisA      = glm::vec2( 0.0, 2.0 ) * mc::getPaintRadius();
                glm::vec2 basisB      = glm::vec2( 2.0, 0.0 ) * mc::getPaintRadius();
                glm::u8vec4 color     = glm::u8vec4( mc::getPaintColor() * 255.0f, 255 );
                mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( mc::UnitSquareMeshIndex );

                app->layers.add( { app->viewParams.mousePos, basisA, basisB, glm::u16vec2( 0 ), glm::u16vec2( 1.0 ), color, mc::HasPillAlphaTex, meshInfo.start,
                                   meshInfo.length, 0, 0 } );
                app->layersModified = true;
            }
            else if( mc::getAppMode() == mc::Mode::Cursor )
            {
                mc::MouseLocationUI mouseLocation = mc::getMouseLocationUI();

                if( mouseLocation == mc::MouseLocationUI::RotateHandle )
                {
                    app->dragType = mc::CursorDragType::Rotate;
                }
                else if( mouseLocation == mc::MouseLocationUI::ScaleHandleTL )
                {
                    app->dragType = mc::CursorDragType::ScaleTL;
                }
                else if( mouseLocation == mc::MouseLocationUI::ScaleHandleBR )
                {
                    app->dragType = mc::CursorDragType::ScaleBR;
                }
                else if( mouseLocation == mc::MouseLocationUI::ScaleHandleTR )
                {
                    app->dragType = mc::CursorDragType::ScaleTR;
                }
                else if( mouseLocation == mc::MouseLocationUI::ScaleHandleBL )
                {
                    app->dragType = mc::CursorDragType::ScaleBL;
                }
                else if( app->selectionBbox.x > app->viewParams.mousePos.x && app->selectionBbox.y > app->viewParams.mousePos.y &&
                         app->selectionBbox.z < app->viewParams.mousePos.x && app->selectionBbox.w < app->viewParams.mousePos.y )
                {
                    app->dragType = mc::CursorDragType::Move;
                }
                else
                {
                    app->dragType = mc::CursorDragType::Select;
                }
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:

        // click selection if the mouse hasnt moved since mouse down
        if( app->mouseWindowPos == app->mouseDragStart && mc::getAppMode() == mc::Mode::Cursor )
        {
            app->viewParams.selectDispatch = mc::SelectDispatch::Point;
        }
        else if( app->dragType != mc::CursorDragType::Select )
        {
            // dispatch selection compute after a tranform to recalculate bboxes without modyfying selection
            app->viewParams.selectDispatch = mc::SelectDispatch::ComputeBbox;
        }
        app->mouseDown = false;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app->mouseWindowPos = glm::vec2( event->motion.x, event->motion.y );
        app->mouseDelta += glm::vec2( event->motion.xrel, event->motion.yrel );

        if( app->mouseDown && mc::getMouseLocationUI() != mc::MouseLocationUI::Window && mc::getAppMode() == mc::Mode::Pan )
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

    if( app->resetSurface && SDL_GetTicks() - app->resetSurfaceTime > mc::resetSurfaceDelayMs )
    {
        SDL_GetWindowSize( app->window, &app->width, &app->height );
        SDL_GetWindowSizeInPixels( app->window, &app->bbwidth, &app->bbheight );
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        mc::configureSurface( app );
#endif
        app->resetSurface = false;
        app->updateView   = true;
    }

    if( app->resetSurface )
    {
        return SDL_APP_CONTINUE;
    }

    if( app->updateUIStyles )
    {
        mc::setStylesUI( app->dpiFactor );
        app->updateUIStyles = false;
    }

    // Update canvas offset
    if( app->mouseDelta.length() > 0.0 && app->mouseDown && app->updateView && mc::getAppMode() == mc::Mode::Pan )
    {
        app->viewParams.canvasPos += app->mouseDelta;
    }

    // Update mouse position in canvas coordinate space
    app->viewParams.mousePos = ( app->mouseWindowPos - app->viewParams.canvasPos ) / app->viewParams.scale;

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

    if( mc::getAppMode() == mc::Mode::Cursor && app->mouseDown && app->mouseDragStart != app->mouseWindowPos && app->mouseDelta != glm::vec2( 0.0 ) )
    {
        switch( app->dragType )
        {
        case mc::CursorDragType::Select:
        {
            app->viewParams.selectDispatch = mc::SelectDispatch::Box;
            app->viewParams.mouseSelectPos = ( app->mouseDragStart - app->viewParams.canvasPos ) / app->viewParams.scale;
        }
        break;
        case mc::CursorDragType::Move:
        {
            app->layers.moveSelection( app->mouseDelta / app->viewParams.scale );
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
            glm::vec2 mouseDeltaCanvas = app->mouseDelta / app->viewParams.scale;
            glm::vec2 factor = mouseDeltaCanvas / glm::vec2( app->selectionBbox.x - app->selectionBbox.z, app->selectionBbox.y - app->selectionBbox.w );

            glm::vec2 orientation = glm::vec2( 1.0 );
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

            // Update bounding box
            app->selectionBbox.x += mouseDeltaCanvas.x * orientation.x;
            app->selectionBbox.y += mouseDeltaCanvas.y * orientation.y;
            app->selectionBbox.z -= mouseDeltaCanvas.x * orientation.x;
            app->selectionBbox.w -= mouseDeltaCanvas.y * orientation.y;

            app->layers.scaleSelection( app->selectionCenter, 2.0f * factor * orientation + glm::vec2( 1.0 ) );
            app->layersModified = true;
        }
        break;
        }
    }
    if( mc::getAppMode() == mc::Mode::Paint && app->mouseDown && app->mouseDelta != glm::vec2( 0.0 ) )
    {
        glm::vec2 basisA      = app->mouseDelta / app->viewParams.scale + 2.0f * glm::normalize( app->mouseDelta ) * mc::getPaintRadius();
        glm::vec2 basisB      = 2.0f * glm::normalize( glm::vec2( app->mouseDelta.y, -app->mouseDelta.x ) ) * mc::getPaintRadius();
        glm::u8vec4 color     = glm::u8vec4( mc::getPaintColor() * 255.0f, 255 );
        mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( mc::UnitSquareMeshIndex );

        app->layers.add( {
            app->viewParams.mousePos - app->mouseDelta / app->viewParams.scale * 0.5f,
            basisA,
            basisB,
            glm::u16vec2( 0 ),
            glm::u16vec2( 1.0 ),
            color,
            mc::HasPillAlphaTex,
            meshInfo.start,
            meshInfo.length,
            0,
            0,
        } );
        app->layersModified = true;
    }
    if( app->layersModified )
    {
        app->viewParams.numLayers = static_cast<uint32_t>( app->layers.length() );
    }
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
            if( i < app->layerEditStart )
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
            submitEvent( mc::Events::ContextCancel );
            app->mergeTopLayers = false;
        }
        else
        {
            encoder.CopyBufferToBuffer( app->vertexBuf, newMeshOffset, app->vertexCopyBuf, 0, app->newMeshSize );
        }
    }

    wgpu::SurfaceTexture surfaceTexture;
    app->surface.GetCurrentTexture( &surfaceTexture );

    wgpu::RenderPassColorAttachment renderPassColorAttachment;
    renderPassColorAttachment.view    = surfaceTexture.texture.CreateView();
    renderPassColorAttachment.loadOp  = wgpu::LoadOp::Clear;
    renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
    renderPassColorAttachment.clearValue =
        wgpu::Color{ Spectrum::ColorR( Spectrum::Static::BONE ), Spectrum::ColorG( Spectrum::Static::BONE ), Spectrum::ColorB( Spectrum::Static::BONE ), 1.0f };


    wgpu::RenderPassDescriptor renderPassDesc;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments     = &renderPassColorAttachment;

    wgpu::RenderPassEncoder renderPassEnc = encoder.BeginRenderPass( &renderPassDesc );

    if( app->layers.length() > 0 )
    {
        renderPassEnc.SetPipeline( app->mainPipeline );
        renderPassEnc.SetVertexBuffer( 0, app->vertexBuf );
        renderPassEnc.SetVertexBuffer( 1, app->layerBuf );
        renderPassEnc.SetBindGroup( 0, app->globalBindGroup );

        // webgpu doesnt have texture arrays or bindless textures so we cant use batch rendering
        // for now draw each layer with a seperate command
        int offset = 0;
        for( int i = 0; i < app->layers.length(); ++i )
        {
            app->textureManager.bind( app->layers.data()[i].texture, 1, renderPassEnc );
            renderPassEnc.Draw( app->layers.data()[i].vertexBuffLength * 3, 1, offset );
            offset += app->layers.data()[i].vertexBuffLength * 3;
        }
    }

    mc::drawUI( app, renderPassEnc );

    renderPassEnc.End();

    // We only want to recompute the selection array if a selection is requested and any
    // previously requested computations have completed aka selection ready
    bool computeSelection = app->viewParams.selectDispatch != mc::SelectDispatch::None && app->selectionReady && app->layers.length() > 0;
    if( computeSelection )
    {
        encoder.ClearBuffer( app->selectionBuf, 0, app->layers.length() * sizeof( mc::Selection ) );
        wgpu::ComputePassEncoder computePassEnc = encoder.BeginComputePass();
        computePassEnc.SetPipeline( app->selectionPipeline );
        computePassEnc.SetBindGroup( 0, app->globalBindGroup );
        computePassEnc.SetBindGroup( 1, app->meshBindGroup );
        computePassEnc.SetBindGroup( 2, app->selectionBindGroup );

        computePassEnc.DispatchWorkgroups( ( app->layers.getTotalTriCount() + 256 - 1 ) / 256, 1, 1 );
        computePassEnc.End();

        encoder.CopyBufferToBuffer( app->selectionBuf, 0, app->selectionMapBuf, 0, app->layers.getTotalTriCount() * sizeof( mc::Selection ) );
    }

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label   = "Melchior";
    wgpu::CommandBuffer command = encoder.Finish( &cmdBufferDescriptor );

    app->device.GetQueue().Submit( 1, &command );

    // Reset
    app->mouseDelta  = glm::vec2( 0.0 );
    app->scrollDelta = glm::vec2( 0.0 );

    if( app->mergeTopLayers )
    {
        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userData )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                submitEvent( mc::Events::MergeTopLayers );
            }
            else
            {
                submitEvent( mc::Events::ContextCancel );
            }
        };
        app->vertexCopyBuf.MapAsync( wgpu::MapMode::Read, 0, app->newMeshSize, callback, &app->vertexCopyBuf );

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
        app->selectionMapBuf.MapAsync( wgpu::MapMode::Read, 0, app->layers.getTotalTriCount() * sizeof( mc::Selection ), callback, &app->selectionMapBuf );

        app->selectionReady = false;
    }

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->surface.Present();
    app->device.Tick();
#endif

    return app->appQuit ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

void SDL_AppQuit( void* appstate )
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
