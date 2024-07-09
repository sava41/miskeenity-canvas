#include "sdl_utils.h"

#include "image.h"
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
#include "window_icon.h"
#endif

#include <SDL3/SDL_misc.h>

namespace mc
{

    void setMouseCursor()
    {
    }

    void setWindowIcon( SDL_Window* window )
    {
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        int width, height;

        ImageData pixels = loadImageFromBuffer( miskeen_32_png, miskeen_32_png_size, width, height );

        SDL_Surface* icon = SDL_CreateSurfaceFrom( pixels.get(), width, height, width * sizeof( uint32_t ), SDL_PIXELFORMAT_RGBA8888 );

        SDL_SetWindowIcon( window, icon );

        SDL_DestroySurface( icon );
#endif
    }

} // namespace mc
