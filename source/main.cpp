#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <imgui.h>
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
    ImGuiIO& io         = ImGui::GetIO();

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
        io.AddMouseButtonEvent( 0, true );

        if( !io.WantCaptureMouse )
        {
            app->mouseDragStart = app->mouseWindowPos;
            if( app->selectionBbox.x > app->viewParams.mousePos.x && app->selectionBbox.y > app->viewParams.mousePos.y &&
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
        io.AddMouseButtonEvent( 0, false );

        // click selection if the mouse hasnt moved since mouse down
        if( app->mouseWindowPos == app->mouseDragStart )
        {
            // TODO
        }
        else if( app->dragType == mc::CursorDragType::Move || app->dragType == mc::CursorDragType::Scale || app->dragType == mc::CursorDragType::Rotate )
        {
            // dispatch selection compute to recalculate bboxes without modyfying selection
            app->viewParams.mouseSelectPos = glm::vec2( 0.0 );
            app->selectionRequested        = true;
        }
        app->mouseDown = false;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        io.AddMousePosEvent( event->motion.x, event->motion.y );
        app->mouseWindowPos = glm::vec2( event->motion.x, event->motion.y );
        app->mouseDelta += glm::vec2( event->motion.xrel, event->motion.yrel );

        if( app->mouseDown && !io.WantCaptureMouse && app->state == mc::State::Pan )
        {
            app->updateView = true;
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        io.AddMouseWheelEvent( event->wheel.x, event->wheel.y );
        app->scrollDelta += glm::vec2( event->wheel.x, event->wheel.y );

        if( !io.WantCaptureMouse )
        {
            app->updateView = true;
        }
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
            app->selectionRequested        = true;
            app->viewParams.mouseSelectPos = ( app->mouseDragStart - app->viewParams.canvasPos ) / app->viewParams.scale;
            break;
        case mc::CursorDragType::Move:
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

        app->layers.add( { pos, glm::vec2( 100, 0 ), glm::vec2( 0, 100 ), glm::u16vec2( 0 ), glm::u16vec2( 0 ), 0, 0, glm::u8vec3( red, green, blue ),
                           mc::LayerType::Textured } );

        app->layersModified = true;

        app->addLayer = false;
    }

    if( app->mouseDelta.length() > 0.0 && app->mouseDown && app->state == mc::State::Cursor )
    {
        switch( app->dragType )
        {
        case mc::CursorDragType::Move:
            app->layers.moveSelection( app->mouseDelta / app->viewParams.scale );
            break;
        }
        app->layersModified = true;
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
    if( app->selectionRequested && app->selectionReady && app->layers.length() > 0 )
    {
        encoder.ClearBuffer( app->selectionBuf, 0, app->layers.length() * sizeof( uint32_t ) );
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline( app->selectionPipeline );
        // figure out which things to bind;
        computePass.SetBindGroup( 0, app->bindGroup );
        computePass.SetBindGroup( 1, app->selectionBindGroup );

        computePass.DispatchWorkgroups( ( app->layers.length() + 256 - 1 ) / 256, 1, 1 );
        computePass.End();

        app->selectionMapBuf.Unmap();
        app->selectionData     = nullptr;
        app->numLayersSelected = 0;

        encoder.CopyBufferToBuffer( app->selectionBuf, 0, app->selectionMapBuf, 0, sizeof( float ) * mc::NumLayers );
    }

    wgpu::CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label   = "Melchior";
    wgpu::CommandBuffer command = encoder.Finish( &cmdBufferDescriptor );

    app->device.GetQueue().Submit( 1, &command );

    if( app->selectionRequested && app->selectionReady && app->layers.length() > 0 )
    {
        app->selectionReady              = false;
        wgpu::BufferMapCallback callback = []( WGPUBufferMapAsyncStatus status, void* userdata )
        {
            if( status == WGPUBufferMapAsyncStatus_Success )
            {
                mc::AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );
                app->selectionData  = reinterpret_cast<mc::Selection*>(
                    const_cast<void*>( ( app->selectionMapBuf.GetConstMappedRange( 0, sizeof( mc::Selection ) * app->layers.length() ) ) ) );

                app->selectionBbox     = glm::vec4( -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                                    std::numeric_limits<float>::max() );
                app->numLayersSelected = 0;
                app->layers.clearSelection();

                for( int i = 0; i < app->layers.length(); ++i )
                {
                    if( app->selectionData[i].flags != mc::SelectionFlags::InsideBox )
                    {
                        continue;
                    }

                    app->selectionBbox.x = std::max( app->selectionBbox.x, app->selectionData[i].bbox.x );
                    app->selectionBbox.y = std::max( app->selectionBbox.y, app->selectionData[i].bbox.y );
                    app->selectionBbox.z = std::min( app->selectionBbox.z, app->selectionData[i].bbox.z );
                    app->selectionBbox.w = std::min( app->selectionBbox.w, app->selectionData[i].bbox.w );

                    app->numLayersSelected += 1;
                    app->layers.addSelection( i );

                    app->dragType = mc::CursorDragType::Select;
                }

                app->selectionReady = true;
            }
        };
        app->selectionMapBuf.MapAsync( wgpu::MapMode::Read, 0, sizeof( float ) * mc::NumLayers, callback, app );
    }

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
    app->swapchain.Present();
    app->device.Tick();
#endif

    // Reset
    app->mouseDelta         = glm::vec2( 0.0 );
    app->scrollDelta        = glm::vec2( 0.0 );
    app->updateView         = false;
    app->layersModified     = false;
    app->selectionRequested = false;

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
