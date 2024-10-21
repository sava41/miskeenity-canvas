#include "sdl_utils.h"

#include "image.h"
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
#include "battery/embed.hpp"
#endif

#include <SDL3/SDL_misc.h>

namespace mc
{
    void setWindowIcon( SDL_Window* window )
    {
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        int width, height;

        ImageData pixels = loadImageFromBuffer( b::embed<"./resources/textures/miskeen_128.png">().data(),
                                                b::embed<"./resources/textures/miskeen_128.png">().size(), width, height );

        SDL_Surface* icon = SDL_CreateSurfaceFrom( width, height, SDL_PIXELFORMAT_RGBA8888, pixels.get(), width * sizeof( uint32_t ) );
        SDL_SetSurfaceColorspace( icon, SDL_COLORSPACE_SRGB_LINEAR );

        SDL_SetWindowIcon( window, icon );

        SDL_DestroySurface( icon );
#endif
    }

} // namespace mc
