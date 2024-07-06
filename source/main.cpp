#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
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
#include "embedded_files.h"
#include "events.h"
#include "graphics.h"
#include "image.h"
#include "layers.h"
#include "sdl_utils.h"
#include "ui.h"
#include "webgpu_surface.h"

int SDL_Fail()
{
    SDL_LogError( SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError() );
    return -1;
}

int SDL_AppInit( void** appstate, int argc, char* argv[] )
{
    static mc::AppContext appContext;
    mc::AppContext* app = &appContext;
    *appstate           = app;

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

    return 0;
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
        mc::Selection* selectionData = reinterpret_cast<mc::Selection*>( event->user.data1 );

        app->selectionBbox = glm::vec4( -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                        std::numeric_limits<float>::max() );

        int numSelected = 0;

        for( int i = app->layers.length() - 1; i >= 0; --i )
        {
            if( ( app->viewParams.selectDispatch != mc::SelectDispatch::ComputeBbox && selectionData[i].flags != mc::SelectionFlags::InsideBox ) ||
                ( app->viewParams.selectDispatch == mc::SelectDispatch::Point && numSelected == 1 ) )
            {
                app->layers.changeSelection( i, false );
                continue;
            }

            if( app->viewParams.selectDispatch != mc::SelectDispatch::ComputeBbox || app->layers.isSelected( i ) )
            {
                app->selectionBbox.x = std::max( app->selectionBbox.x, selectionData[i].bbox.x );
                app->selectionBbox.y = std::max( app->selectionBbox.y, selectionData[i].bbox.y );
                app->selectionBbox.z = std::min( app->selectionBbox.z, selectionData[i].bbox.z );
                app->selectionBbox.w = std::min( app->selectionBbox.w, selectionData[i].bbox.w );

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
    }
}

int SDL_AppEvent( void* appstate, const SDL_Event* event )
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
        app->resetSurface = true;
        break;
    case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        app->dpiFactor = SDL_GetWindowDisplayScale( app->window );
        mc::setStylesUI( app->dpiFactor );
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:

        if( mc::getMouseLocationUI() != mc::MouseLocationUI::Window )
        {
            app->mouseDragStart = app->mouseWindowPos;
            app->mouseDown      = true;

            if( mc::getAppMode() == mc::Mode::Paint )
            {
                glm::vec2 basisA  = glm::vec2( 0.0, 2.0 ) * mc::getPaintRadius();
                glm::vec2 basisB  = glm::vec2( 2.0, 0.0 ) * mc::getPaintRadius();
                glm::u8vec4 color = glm::u8vec4( mc::getPaintColor() * 255.0f, 1.0 );

                app->layers.add( { app->viewParams.mousePos, basisA, basisB, glm::u16vec2( 0 ), glm::u16vec2( 1.0 ), static_cast<uint16_t>( 0 ), 0, color,
                                   mc::HasPillAlphaTex } );
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

    return 0;
}

int SDL_AppIterate( void* appstate )
{
    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );

    if( app->resetSurface )
    {
        SDL_GetWindowSize( app->window, &app->width, &app->height );
        SDL_GetWindowSizeInPixels( app->window, &app->bbwidth, &app->bbheight );
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        mc::configureSurface( app );
#endif
        app->resetSurface = false;
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
        glm::vec2 basisA  = app->mouseDelta / app->viewParams.scale + 2.0f * glm::normalize( app->mouseDelta ) * mc::getPaintRadius();
        glm::vec2 basisB  = 2.0f * glm::normalize( glm::vec2( app->mouseDelta.y, -app->mouseDelta.x ) ) * mc::getPaintRadius();
        glm::u8vec4 color = glm::u8vec4( mc::getPaintColor() * 255.0f, 1.0 );

        app->layers.add( { app->viewParams.mousePos - app->mouseDelta / app->viewParams.scale * 0.5f, basisA, basisB, glm::u16vec2( 0 ), glm::u16vec2( 1.0 ),
                           static_cast<uint16_t>( 0 ), 0, color, mc::HasPillAlphaTex } );
        app->layersModified = true;
    }
    app->device.GetQueue().WriteBuffer( app->viewParamBuf, 0, &app->viewParams, sizeof( mc::Uniforms ) );

    if( app->layersModified )
    {
        app->device.GetQueue().WriteBuffer( app->layerBuf, 0, app->layers.data(), app->layers.length() * sizeof( mc::Layer ) );
        app->layersModified = false;
    }

    wgpu::CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Casper";

    wgpu::CommandEncoder encoder = app->device.CreateCommandEncoder( &commandEncoderDesc );

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
        for( int i = 0; i < app->layers.length(); ++i )
        {
            app->textureManager.bind( app->layers.data()[i].texture, 1, renderPassEnc );
            renderPassEnc.Draw( 6, 1, 0, i );
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
        computePassEnc.SetBindGroup( 1, app->selectionBindGroup );

        computePassEnc.DispatchWorkgroups( ( app->layers.length() + 256 - 1 ) / 256, 1, 1 );
        computePassEnc.End();

        encoder.CopyBufferToBuffer( app->selectionBuf, 0, app->selectionMapBuf, 0, app->layers.length() * sizeof( mc::Selection ) );
    }

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label   = "Melchior";
    wgpu::CommandBuffer command = encoder.Finish( &cmdBufferDescriptor );

    app->device.GetQueue().Submit( 1, &command );

    // Reset
    app->mouseDelta  = glm::vec2( 0.0 );
    app->scrollDelta = glm::vec2( 0.0 );

    if( computeSelection )
    {
        app->selectionReady = false;

        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userData )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                wgpu::Buffer* buffer      = reinterpret_cast<wgpu::Buffer*>( userData );
                const void* selectionData = ( buffer->GetConstMappedRange( 0, mc::NumLayers * sizeof( mc::Selection ) ) );

                if( selectionData == nullptr )
                    return;

                submitEvent( mc::Events::SelectionChanged, const_cast<void*>( selectionData ) );
            }
        };
        app->selectionMapBuf.MapAsync( wgpu::MapMode::Read, 0, mc::NumLayers * sizeof( mc::Selection ), callback, &app->selectionMapBuf );
    }

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->surface.Present();
    app->device.Tick();
#endif

    return app->appQuit;
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
