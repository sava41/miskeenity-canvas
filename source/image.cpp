#include "image.h"

#include "app.h"
#include "graphics.h"

#if defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten_browser_file.h>
#else
#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace mc
{
    void addImageToLayer( AppContext* app, unsigned char* imageData, int width, int height, int channels )
    {
        if( imageData != nullptr )
        {

            if( !app->textureManager.add( imageData, width, height, channels, app->device ) )
            {
                stbi_image_free( imageData );
                return;
            }

            app->device.GetQueue().OnSubmittedWorkDone( []( WGPUQueueWorkDoneStatus status, void* imageData ) { stbi_image_free( imageData ); }, imageData );

            glm::vec2 pos = ( glm::vec2( app->width / 2.0, app->height / 2.0 ) - app->viewParams.canvasPos ) / app->viewParams.scale;

            app->layers.add( { pos, glm::vec2( width, 0 ), glm::vec2( 0, height ), glm::u16vec2( 0 ), glm::u16vec2( 1.0 ),
                               static_cast<uint16_t>( app->textureManager.length() - 1 ), 0, glm::u8vec4( 255, 255, 255, 255 ), mc::HasColorTex } );

            app->layersModified = true;

            SDL_Log( "loaded image with width %d and height %d", width, height );
        }
    }

    void loadImageFromFileDialog( AppContext* app )
    {
#if defined( SDL_PLATFORM_EMSCRIPTEN )
        emscripten_browser_file::upload(
            ".png,.jpg,.jpeg",
            []( std::string const& filename, std::string const& mime_type, std::string_view buffer, void* userdata )
            {
                AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );

                if( buffer.size() > 0 )
                {

                    int width, height, channels;
                    unsigned char* imageData =
                        stbi_load_from_memory( reinterpret_cast<const unsigned char*>( buffer.data() ), buffer.size(), &width, &height, nullptr, 4 );

                    addImageToLayer( app, imageData, width, height, channels );
                }
            },
            app );
#else
        static SDL_DialogFileFilter filters[] = {
            { "All Supported Types", "jpg;jpeg;png" },
            { "JPEG (*.jpg;*.jpeg)", "jpg;jpeg" },
            { "PNG (*.png)", "png" },
            { nullptr, nullptr },
        };

        SDL_ShowOpenFileDialog(
            []( void* userdata, const char* const* filelist, int filter )
            {
                AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );

                if( filelist && filelist[0] )
                {
                    int width, height, channels;
                    unsigned char* imageData = stbi_load( filelist[0], &width, &height, &channels, 4 );

                    addImageToLayer( app, imageData, width, height, channels );
                }
            },
            app, nullptr, filters, nullptr, SDL_FALSE );

#endif
    }


    ImageData loadImageFromBuffer( const void* buffer, int len, int& width, int& height )
    {
        return std::move( ImageData( stbi_load_from_memory( static_cast<const unsigned char*>( buffer ), len, &width, &height, nullptr, 4 ),
                                     []( unsigned char* data ) { stbi_image_free( data ); } ) );
    }
} // namespace mc