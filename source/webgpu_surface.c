#include "webgpu_surface.h"

#if defined( SDL_PLATFORM_MACOS )
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#elif defined( SDL_PLATFORM_LINUX )

#elif defined( SDL_PLATFORM_WIN32 )

#elif defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <SDL3/SDL.h>

WGPUSurface SDL_GetWGPUSurface( WGPUInstance instance, SDL_Window* window )
{
    SDL_PropertiesID props = SDL_GetWindowProperties( window );

#if defined( SDL_PLATFORM_MACOS )
    {
        id metal_layer      = NULL;
        NSWindow* ns_window = (__bridge NSWindow*)SDL_GetPointerProperty( props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL );
        [ns_window.contentView setWantsLayer:YES];
        metal_layer = [CAMetalLayer layer];
        [ns_window.contentView setLayer:metal_layer];
        return wgpuInstanceCreateSurface( instance, &(WGPUSurfaceDescriptor){
                                                        .label = NULL,
                                                        .nextInChain =
                                                            (const WGPUChainedStruct*)&(WGPUSurfaceSourceMetalLayer){
                                                                .chain =
                                                                    (WGPUChainedStruct){
                                                                        .next  = NULL,
                                                                        .sType = WGPUSType_SurfaceSourceMetalLayer,
                                                                    },
                                                                .layer = metal_layer,
                                                            },
                                                    } );
    }
#elif defined( SDL_PLATFORM_LINUX )
    {
        if( SDL_strcmp( SDL_GetCurrentVideoDriver(), "x11" ) == 0 )
        {
            return wgpuInstanceCreateSurface( instance, &(WGPUSurfaceDescriptor){
                                                            .label = NULL,
                                                            .nextInChain =
                                                                (const WGPUChainedStruct*)&(WGPUSurfaceSourceXlibWindow){
                                                                    .chain =
                                                                        (WGPUChainedStruct){
                                                                            .next  = NULL,
                                                                            .sType = WGPUSType_SurfaceSourceXlibWindow,
                                                                        },
                                                                    .display = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL ),
                                                                    .window  = SDL_GetNumberProperty( props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0 ),
                                                                },
                                                        } );
        }
        else if( SDL_strcmp( SDL_GetCurrentVideoDriver(), "wayland" ) == 0 )
        {
            return wgpuInstanceCreateSurface( instance, &(WGPUSurfaceDescriptor){
                                                            .label = NULL,
                                                            .nextInChain =
                                                                (const WGPUChainedStruct*)&(WGPUSurfaceSourceWaylandSurface){
                                                                    .chain =
                                                                        (WGPUChainedStruct){
                                                                            .next  = NULL,
                                                                            .sType = WGPUSType_SurfaceSourceWaylandSurface,
                                                                        },
                                                                    .display = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL ),
                                                                    .surface = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL ),
                                                                },
                                                        } );
        }
    }
#elif defined( SDL_PLATFORM_WIN32 )
    {
        return wgpuInstanceCreateSurface( instance, &(WGPUSurfaceDescriptor){
                                                        .label = NULL,
                                                        .nextInChain =
                                                            (const WGPUChainedStruct*)&(WGPUSurfaceSourceWindowsHWND){
                                                                .chain =
                                                                    (WGPUChainedStruct){
                                                                        .next  = NULL,
                                                                        .sType = WGPUSType_SurfaceSourceWindowsHWND,
                                                                    },
                                                                .hinstance = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, NULL ),
                                                                .hwnd      = SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL ),
                                                            },
                                                    } );
    }
#elif defined( SDL_PLATFORM_EMSCRIPTEN )

    return wgpuInstanceCreateSurface( instance, &(WGPUSurfaceDescriptor){
                                                    .label = NULL,
                                                    .nextInChain =
                                                        (const WGPUChainedStruct*)&(WGPUEmscriptenSurfaceSourceCanvasHTMLSelector){
                                                            .chain =
                                                                (WGPUChainedStruct){
                                                                    .next  = NULL,
                                                                    .sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector,
                                                                },
                                                            .selector = HTML_CANVAS_ID,
                                                        },
                                                } );
#endif
}