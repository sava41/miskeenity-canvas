#include "image.h"

#include "app.h"
#include "webgpu_utils.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace mc
{
    constexpr static SDL_DialogFileFilter filters[] = {
        { "JPEG (*.jpg;*.jpeg)", "jpg;jpeg" },
        { "PNG (*.png)", "png" },
        { nullptr, nullptr },
    };

    void loadImageFromFile( AppContext* app )
    {

        SDL_ShowOpenFileDialog(
            []( void* userdata, const char* const* filelist, int filter )
            {
                AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );

                if( filelist && filelist[0] )
                {
                    int width, height, channels;
                    unsigned char* image = stbi_load( filelist[0], &width, &height, &channels, 4 );

                    if( image != nullptr )
                    {
                        if( app->textureData != nullptr )
                        {
                            stbi_image_free( app->textureData );
                            app->textureData = nullptr;
                        }

                        app->textureData = image;

                        createTexture( app, width, height );
                        uploadTexture( app->device.GetQueue(), app->texture, app->textureData, width, height, 4 );

                        // temporary generate random color
                        const uint32_t a = 1664525;
                        const uint32_t c = 1013904223;

                        const uint32_t color = a * app->layers.length() + c;
                        const uint8_t red    = static_cast<uint8_t>( ( color >> 16 ) & 0xFF );
                        const uint8_t green  = static_cast<uint8_t>( ( color >> 8 ) & 0xFF );
                        const uint8_t blue   = static_cast<uint8_t>( color & 0xFF );

                        glm::vec2 pos = ( glm::vec2( app->width / 2.0, app->height / 2.0 ) - app->viewParams.canvasPos ) / app->viewParams.scale;

                        app->layers.add( { pos, glm::vec2( width, 0 ), glm::vec2( 0, height ), glm::u16vec2( 0 ), glm::u16vec2( 0 ), 0, 0,
                                           glm::u8vec4( red, green, blue, 255 ), 0 } );


                        app->layersModified = true;

                        SDL_Log( "loaded image `%s` with width %d and height %d", filelist[0], width, height );
                    }
                }
            },
            app, nullptr, filters, nullptr, SDL_FALSE );
    }
} // namespace mc