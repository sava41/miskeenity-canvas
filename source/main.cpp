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
#include "embedded_files.h"
#include "layers.h"
#include "ui.h"
#include "webgpu_surface.h"
#include "webgpu_utils.h"

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

    wgpu::RequestAdapterOptions adapterOpts;
    adapterOpts.compatibleSurface = app->surface;

    app->adapter = mc::requestAdapter( app->instance, &adapterOpts );
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
    app->device                   = mc::requestDevice( app->adapter, &deviceDesc );

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

    SDL_GetWindowSize( app->window, &app->width, &app->height );
    SDL_GetWindowSizeInPixels( app->window, &app->bbwidth, &app->bbheight );
    mc::initSwapChain( app );

    mc::initUI( app );

    initMainPipeline( app );

    // print some information about the window
    SDL_ShowWindow( app->window );
    {
        SDL_Log( "Window size: %ix%i", app->width, app->height );
        SDL_Log( "Backbuffer size: %ix%i", app->bbwidth, app->bbheight );
    }

    app->updateView = true;

    SDL_Log( "Application started successfully!" );

    return 0;
}

int SDL_AppEvent( void* appstate, const SDL_Event* event )
{
    mc::AppContext* app = reinterpret_cast<mc::AppContext*>( appstate );

    mc::processEventUI( event );

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

        if( !mc::captureMouseUI() )
        {
            app->mouseDragStart = app->mouseWindowPos;

            glm::vec2 cornerTL = glm::vec2( app->selectionBbox.z, app->selectionBbox.w ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerBR = glm::vec2( app->selectionBbox.x, app->selectionBbox.y ) * app->viewParams.scale + app->viewParams.canvasPos;
            glm::vec2 cornerTR = glm::vec2( cornerBR.x, cornerTL.y );
            glm::vec2 cornerBL = glm::vec2( cornerTL.x, cornerBR.y );

            float screenSpaceCenterX = ( cornerTL.x + cornerBR.x ) * 0.5f;

            glm::vec2 rotHandlePos = glm::vec2( screenSpaceCenterX, cornerTR.y - mc::RotateHandleHeight );

            if( app->layers.numSelected() > 0 && glm::distance( app->mouseWindowPos, rotHandlePos ) < mc::HandleHalfSize )
            {
                app->dragType = mc::CursorDragType::Rotate;
            }
            else if( app->layers.numSelected() > 0 && glm::distance( app->mouseWindowPos, cornerTL ) < mc::HandleHalfSize )
            {
                app->dragType = mc::CursorDragType::ScaleTL;
            }
            else if( app->layers.numSelected() > 0 && glm::distance( app->mouseWindowPos, cornerBR ) < mc::HandleHalfSize )
            {
                app->dragType = mc::CursorDragType::ScaleBR;
            }
            else if( app->layers.numSelected() > 0 && glm::distance( app->mouseWindowPos, cornerTR ) < mc::HandleHalfSize )
            {
                app->dragType = mc::CursorDragType::ScaleTR;
            }
            else if( app->layers.numSelected() > 0 && glm::distance( app->mouseWindowPos, cornerBL ) < mc::HandleHalfSize )
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

            app->mouseDown = true;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:

        // click selection if the mouse hasnt moved since mouse down
        if( app->mouseWindowPos == app->mouseDragStart )
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

        if( app->mouseDown && !mc::captureMouseUI() && app->state == mc::State::Pan )
        {
            app->updateView = true;
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        app->scrollDelta += glm::vec2( event->wheel.x, event->wheel.y );

        if( !mc::captureMouseUI() )
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

    if( app->resetSwapchain )
    {
        SDL_GetWindowSize( app->window, &app->width, &app->height );
        SDL_GetWindowSizeInPixels( app->window, &app->bbwidth, &app->bbheight );
        mc::initSwapChain( app );

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
    app->device.GetQueue().WriteBuffer( app->viewParamBuf, 0, &app->viewParams, sizeof( mc::Uniforms ) );

    if( app->addLayer )
    {
        // temporary generate random color
        const uint32_t a = 1664525;
        const uint32_t c = 1013904223;

        const uint32_t color = a * app->layers.length() + c;
        const uint8_t red    = static_cast<uint8_t>( ( color >> 16 ) & 0xFF );
        const uint8_t green  = static_cast<uint8_t>( ( color >> 8 ) & 0xFF );
        const uint8_t blue   = static_cast<uint8_t>( color & 0xFF );

        // place at screen center
        glm::vec2 pos = ( glm::vec2( app->width / 2.0, app->height / 2.0 ) - app->viewParams.canvasPos ) / app->viewParams.scale;

        app->layers.add(
            { pos, glm::vec2( 100, 0 ), glm::vec2( 0, 100 ), glm::u16vec2( 0 ), glm::u16vec2( 0 ), 0, 0, glm::u8vec4( red, green, blue, 255 ), 0 } );

        app->layersModified = true;

        app->addLayer = false;
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
    if( app->viewParams.selectDispatch != mc::SelectDispatch::None && app->selectionReady && app->layers.length() > 0 )
    {
        encoder.ClearBuffer( app->selectionBuf, 0, app->layers.length() * sizeof( mc::Selection ) );
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline( app->selectionPipeline );
        // figure out which things to bind;
        computePass.SetBindGroup( 0, app->bindGroup );
        computePass.SetBindGroup( 1, app->selectionBindGroup );

        computePass.DispatchWorkgroups( ( app->layers.length() + 256 - 1 ) / 256, 1, 1 );
        computePass.End();

        encoder.CopyBufferToBuffer( app->selectionBuf, 0, app->selectionMapBuf, 0, app->layers.length() * sizeof( mc::Selection ) );
    }

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label   = "Melchior";
    wgpu::CommandBuffer command = encoder.Finish( &cmdBufferDescriptor );

    app->device.GetQueue().Submit( 1, &command );

    // Reset
    app->mouseDelta     = glm::vec2( 0.0 );
    app->scrollDelta    = glm::vec2( 0.0 );
    app->updateView     = false;
    app->layersModified = false;

    if( app->viewParams.selectDispatch != mc::SelectDispatch::None && app->selectionReady && app->layers.length() > 0 )
    {
        app->selectionReady = false;

        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userData )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                mc::AppContext* app = reinterpret_cast<mc::AppContext*>( userData );
                const mc::Selection* selectionData =
                    reinterpret_cast<const mc::Selection*>( ( app->selectionMapBuf.GetConstMappedRange( 0, app->layers.length() * sizeof( mc::Selection ) ) ) );

                if( selectionData == nullptr )
                    return;

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

                app->selectionCenter =
                    ( glm::vec2( app->selectionBbox.x, app->selectionBbox.y ) + glm::vec2( app->selectionBbox.z, app->selectionBbox.w ) ) * 0.5f;

                app->selectionReady            = true;
                app->layersModified            = true;
                app->viewParams.selectDispatch = mc::SelectDispatch::None;
                app->dragType                  = mc::CursorDragType::Select;

                app->selectionMapBuf.Unmap();
            }
        };
        app->selectionMapBuf.MapAsync( wgpu::MapMode::Read, 0, app->layers.length() * sizeof( mc::Selection ), callback, app );
    }

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->swapchain.Present();
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
