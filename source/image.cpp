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
                    unsigned char* imageData = stbi_load( filelist[0], &width, &height, &channels, 4 );

                    if( imageData != nullptr )
                    {

                        app->device.GetQueue().OnSubmittedWorkDone(
                            0, []( WGPUQueueWorkDoneStatus status, void* imageData ) { stbi_image_free( imageData ); }, imageData );

                        if( !app->textureManager.add( imageData, width, height, channels, app->device ) )
                        {
                            stbi_image_free( imageData );
                            return;
                        }

                        glm::vec2 pos = ( glm::vec2( app->width / 2.0, app->height / 2.0 ) - app->viewParams.canvasPos ) / app->viewParams.scale;

                        app->layers.add( { pos, glm::vec2( width, 0 ), glm::vec2( 0, height ), glm::u16vec2( 0 ), glm::u16vec2( 1.0 ),
                                           static_cast<uint16_t>( app->textureManager.length() - 1 ), 0, glm::u8vec4( 255, 255, 255, 255 ), 0 } );

                        app->layersModified = true;

                        SDL_Log( "loaded image `%s` with width %d and height %d", filelist[0], width, height );
                    }
                }
            },
            app, nullptr, filters, nullptr, SDL_FALSE );
    }
} // namespace mc