#pragma once

#include <memory>
#include <string>

namespace mc
{
    struct AppContext;
    using ImageData = std::unique_ptr<unsigned char, void ( * )( unsigned char* )>;

    void loadImageFromFileDialog( AppContext* app );

    void addImageLayerFromFile( AppContext* app, const std::string& filepath );

    ImageData loadImageFromBuffer( const void* buffer, int len, int& width, int& height );

    void saveImageFromFileDialog( AppContext* app );

} // namespace mc