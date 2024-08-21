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
#include <emscripten/html5_webgpu.h>
#endif

#include <SDL3/SDL.h>

WGPUSurface SDL_GetWGPUSurface( WGPUInstance instance, SDL_Window* window )
{
#if defined( SDL_PLATFORM_MACOS )
    {
        id metal_layer      = NULL;
        NSWindow* ns_window = (__bridge NSWindow*)SDL_GetProperty( SDL_GetWindowProperties( window ), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL );
        [ns_window.contentView setWantsLayer:YES];
        metal_layer = [CAMetalLayer layer];
        [ns_window.contentView setLayer:metal_layer];
        return wgpuInstanceCreateSurface( instance, &( WGPUSurfaceDescriptor ){
                                                        .label = NULL,
                                                        .nextInChain =
                                                            (const WGPUChainedStruct*)&( WGPUSurfaceDescriptorFromMetalLayer ){
                                                                .chain =
                                                                    ( WGPUChainedStruct ){
                                                                        .next  = NULL,
                                                                        .sType = WGPUSType_SurfaceDescriptorFromMetalLayer,
                                                                    },
                                                                .layer = metal_layer,
                                                            },
                                                    } );
    }
#elif defined( SDL_PLATFORM_LINUX )
    {
        if( SDL_strcmp( SDL_GetCurrentVideoDriver(), "x11" ) == 0 )
        {
            return wgpuInstanceCreateSurface(
                instance, &( WGPUSurfaceDescriptor ){
                              .label = NULL,
                              .nextInChain =
                                  (const WGPUChainedStruct*)&( WGPUSurfaceDescriptorFromXlibWindow ){
                                      .chain =
                                          ( WGPUChainedStruct ){
                                              .next  = NULL,
                                              .sType = WGPUSType_SurfaceDescriptorFromXlibWindow,
                                          },
                                      .display = SDL_GetPointerProperty( SDL_GetWindowProperties( window ), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL ),
                                      .window  = SDL_GetNumberProperty( SDL_GetWindowProperties( window ), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0 ),
                                  },
                          } );
        }
        else if( SDL_strcmp( SDL_GetCurrentVideoDriver(), "wayland" ) == 0 )
        {
            return wgpuInstanceCreateSurface(
                instance, &( WGPUSurfaceDescriptor ){
                              .label = NULL,
                              .nextInChain =
                                  (const WGPUChainedStruct*)&( WGPUSurfaceDescriptorFromWaylandSurface ){
                                      .chain =
                                          ( WGPUChainedStruct ){
                                              .next  = NULL,
                                              .sType = WGPUSType_SurfaceDescriptorFromWaylandSurface,
                                          },
                                      .display = SDL_GetPointerProperty( SDL_GetWindowProperties( window ), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL ),
                                      .surface = SDL_GetPointerProperty( SDL_GetWindowProperties( window ), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL ),
                                  },
                          } );
        }
    }
#elif defined( SDL_PLATFORM_WIN32 )
    {
        return wgpuInstanceCreateSurface(
            instance, &( WGPUSurfaceDescriptor ){
                          .label = NULL,
                          .nextInChain =
                              (const WGPUChainedStruct*)&( WGPUSurfaceDescriptorFromWindowsHWND ){
                                  .chain =
                                      ( WGPUChainedStruct ){
                                          .next  = NULL,
                                          .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
                                      },
                                  .hinstance = SDL_GetPointerProperty( SDL_GetWindowProperties( window ), SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, NULL ),
                                  .hwnd      = SDL_GetPointerProperty( SDL_GetWindowProperties( window ), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL ),
                              },
                      } );
    }
#elif defined( SDL_PLATFORM_EMSCRIPTEN )

    return wgpuInstanceCreateSurface( instance, &( WGPUSurfaceDescriptor ){
                                                    .label = NULL,
                                                    .nextInChain =
                                                        (const WGPUChainedStruct*)&( WGPUSurfaceDescriptorFromCanvasHTMLSelector ){
                                                            .chain =
                                                                ( WGPUChainedStruct ){
                                                                    .next  = NULL,
                                                                    .sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector,
                                                                },
                                                            .selector = HTML_CANVAS_ID,
                                                        },
                                                } );
#endif
}