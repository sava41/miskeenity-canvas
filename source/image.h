#pragma once

#include < memory>

namespace mc
{
    struct AppContext;

    void loadImageFromFileDialog( AppContext* app );

    using ImageData = std::unique_ptr<unsigned char, void ( * )( unsigned char* )>;

    ImageData loadImageFromBuffer( const void* buffer, int len, int& width, int& height );

} // namespace mc