#pragma once

#include <memory>

namespace mc
{
    struct AppContext;
    using ImageData = std::unique_ptr<unsigned char, void ( * )( unsigned char* )>;

    void loadImageFromFileDialog( AppContext* app );

    ImageData loadImageFromBuffer( const void* buffer, int len, int& width, int& height );

    void saveImageFromFileDialog( AppContext* app );

} // namespace mc