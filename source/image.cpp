#include "image.h"

#include "app.h"
#include "events.h"
#include "graphics.h"

#if defined( SDL_PLATFORM_EMSCRIPTEN )
#include <emscripten_browser_file.h>
#else
#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_iostream.h>
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

            ResourceHandle rawTextureHandle =
                app->textureManager.add( imageData, width, height, channels, app->device, wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst );
            ResourceHandle processedTextureHandle =
                app->textureManager.add( nullptr, width, height, channels, app->device,
                                         wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::CopySrc, true );

            if( !rawTextureHandle.valid() || !processedTextureHandle.valid() )
            {
                stbi_image_free( imageData );
                return;
            }

            wgpu::CommandEncoderDescriptor commandEncoderDesc;
            commandEncoderDesc.label = "Image";

            wgpu::CommandEncoder encoder = app->device.CreateCommandEncoder( &commandEncoderDesc );

            wgpu::BindGroup inputBindGroup = createComputeTextureBindGroup( app->device, app->textureManager.get( rawTextureHandle ).texture,
                                                                            createReadTextureBindGroupLayout( app->device ) );

            wgpu::BindGroup outputBindGroup = createComputeTextureBindGroup( app->device, app->textureManager.get( processedTextureHandle ).texture,
                                                                             createWriteTextureBindGroupLayout( app->device ) );

            wgpu::ComputePassEncoder computePassEnc = encoder.BeginComputePass();
            computePassEnc.SetPipeline( app->preAlphaPipeline );
            computePassEnc.SetBindGroup( 0, inputBindGroup );
            computePassEnc.SetBindGroup( 1, outputBindGroup );

            computePassEnc.DispatchWorkgroups( ( width + 8 - 1 ) / 8, ( height + 8 - 1 ) / 8, 1 );
            computePassEnc.End();

            wgpu::CommandBufferDescriptor cmdBufferDescriptor;
            cmdBufferDescriptor.label         = "Image Command Buffer";
            wgpu::CommandBuffer imageCommands = encoder.Finish( &cmdBufferDescriptor );
            app->device.GetQueue().Submit( 1, &imageCommands );

            genMipMaps( app->device, app->mipGenPipeline, app->textureManager.get( processedTextureHandle ).texture );

            glm::vec2 pos = ( glm::vec2( app->width / 2.0, app->height / 2.0 ) - app->viewParams.canvasPos ) / app->viewParams.scale;

            MeshInfo meshInfo = app->meshManager.getMeshInfo( UnitSquareMeshIndex );

            app->layers.add( pos, glm::vec2( width, 0 ), glm::vec2( 0, height ), glm::u16vec2( 0 ), glm::u16vec2( UV_MAX_VALUE ),
                             glm::u8vec4( 255, 255, 255, 255 ), HasColorTex, meshInfo, std::move( processedTextureHandle ) );

            app->layerHistory.push( app->layers.createShrunkCopy() );

            app->layersModified = true;

            stbi_image_free( imageData );

            SDL_Log( "loaded image with width %d and height %d", width, height );
        }
    }

    void addImageLayerFromFile( AppContext* app, const std::string& filepath )
    {
        int width, height, channels;
        unsigned char* imageData = stbi_load( filepath.c_str(), &width, &height, &channels, 4 );

        addImageToLayer( app, imageData, width, height, 4 );
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
                if( filelist && filelist[0] )
                {
                    // This is called from another thread so we want to sync by deferring with an event
                    submitEvent( Events::AddImageToLayer, {}, strdup( filelist[0] ) );
                }
            },
            nullptr, nullptr, filters, 3, nullptr, false );

#endif
    }

    ImageData loadImageFromBuffer( const void* buffer, int len, int& width, int& height )
    {
        return std::move( ImageData( stbi_load_from_memory( static_cast<const unsigned char*>( buffer ), len, &width, &height, nullptr, 4 ),
                                     []( unsigned char* data ) { stbi_image_free( data ); } ) );
    }

    void saveImageFromFileDialog( AppContext* app )
    {
#if !defined( SDL_PLATFORM_EMSCRIPTEN )
        static SDL_DialogFileFilter filters[] = { { "PNG (*.png)", "png" } };

        SDL_ShowSaveFileDialog(
            []( void* userdata, const char* const* filelist, int filter )
            {
                AppContext* app = reinterpret_cast<mc::AppContext*>( userdata );
#endif
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

                int length;
                ImageData data = ImageData( stbi_write_png_to_mem( imageData, textureWidthPadded * 4, width, height, 4, &length ),
                                            []( unsigned char* data ) { STBIW_FREE( data ); } );

                app->textureMapBuffer.Unmap();

#if !defined( SDL_PLATFORM_EMSCRIPTEN )
                if( filelist && filelist[0] )
                {
                    std::string filepath( filelist[0] );
                    if( !filepath.ends_with( ".png" ) && !filepath.ends_with( ".PNG" ) )
                    {
                        filepath += ".png";
                    }
                    SDL_IOStream* file = SDL_IOFromFile( filepath.c_str(), "wb" );
                    SDL_WriteIO( file, data.get(), length );
                    SDL_CloseIO( file );
                }
            },
            app, nullptr, filters, 1, nullptr );
#else
        emscripten_browser_file::download( "miskeen.png", "image/png", data.get(), length );
#endif
    }
} // namespace mc