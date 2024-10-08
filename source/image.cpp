#include "image.h"

#include "app.h"
#include "graphics.h"

#if defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten_browser_file.h>
#else
#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#endif

#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace mc
{
    void addImageToLayer( AppContext* app, unsigned char* imageData, int width, int height, int channels )
    {
        if( imageData != nullptr )
        {

            ResourceHandle textureHandle = app->textureManager.add( imageData, width, height, channels, app->device );

            if( !textureHandle.valid() )
            {
                stbi_image_free( imageData );
                return;
            }

            app->device.GetQueue().OnSubmittedWorkDone( []( WGPUQueueWorkDoneStatus status, void* imageData ) { stbi_image_free( imageData ); }, imageData );

            glm::vec2 pos = ( glm::vec2( app->width / 2.0, app->height / 2.0 ) - app->viewParams.canvasPos ) / app->viewParams.scale;

            mc::MeshInfo meshInfo = app->meshManager.getMeshInfo( mc::UnitSquareMeshIndex );

            app->layers.add( pos, glm::vec2( width, 0 ), glm::vec2( 0, height ), glm::u16vec2( 0 ), glm::u16vec2( UV_MAX_VALUE ),
                             glm::u8vec4( 255, 255, 255, 255 ), mc::HasColorTex, meshInfo, std::move( textureHandle ) );

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
                        stbi_load_from_memory( reinterpret_cast<const unsigned char*>( buffer.data() ), buffer.size(), &width, &height, &channels, 4 );

                    addImageToLayer( app, imageData, width, height, 4 );
                }
            },
            app );
#else
        static SDL_DialogFileFilter filters[] = { { "All Supported Types", "jpg;jpeg;png" }, { "JPEG (*.jpg;*.jpeg)", "jpg;jpeg" }, { "PNG (*.png)", "png" } };

        SDL_ShowOpenFileDialog(
            []( void* userdata, const char* const* filelist, int filter )
            {
                AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );

                if( filelist && filelist[0] )
                {
                    int width, height, channels;
                    unsigned char* imageData = stbi_load( filelist[0], &width, &height, &channels, 4 );

                    addImageToLayer( app, imageData, width, height, 4 );
                }
            },
            app, nullptr, filters, 3, nullptr, SDL_FALSE );

#endif
    }

    ImageData loadImageFromBuffer( const void* buffer, int len, int& width, int& height )
    {
        return std::move( ImageData( stbi_load_from_memory( static_cast<const unsigned char*>( buffer ), len, &width, &height, nullptr, 4 ),
                                     []( unsigned char* data ) { stbi_image_free( data ); } ) );
    }

    void saveImageFromFileDialog( AppContext* app )
    {
        static SDL_DialogFileFilter filters[] = { { "PNG (*.png)", "png" } };

        SDL_ShowSaveFileDialog(
            []( void* userdata, const char* const* filelist, int filter )
            {
                AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );

                int width  = app->textureManager.get( *app->copyTextureHandle.get() ).texture.GetWidth();
                int height = app->textureManager.get( *app->copyTextureHandle.get() ).texture.GetHeight();
                app->copyTextureHandle.reset();

                const uint8_t* imageData = reinterpret_cast<const uint8_t*>( app->textureMapBuffer.GetConstMappedRange( 0, app->textureMapBuffer.GetSize() ) );

                if( imageData == nullptr )
                {
                    app->textureMapBuffer.Unmap();
                    return;
                }

                // we need to find the stride since the buffer is padded to be a multiple of 256
                size_t textureWidthPadded = ( width + 256 ) / 256 * 256;

                if( filelist && filelist[0] )
                {
                    stbi_write_png( filelist[0], width, height, 4, imageData, textureWidthPadded * 4 );
                }

                app->textureMapBuffer.Unmap();
            },
            app, nullptr, filters, 1, nullptr );
    }
} // namespace mc