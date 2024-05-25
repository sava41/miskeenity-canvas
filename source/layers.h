#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_precision.hpp>
#include <memory>
#include <vector>

namespace mc
{

    enum class LayerType : uint8_t
    {
        Textured,
        Paint,
        Mask,
        Text
    };

#pragma pack( push, 4 )
    struct Layer
    {
        glm::vec2 offset;
        glm::vec2 basisA;
        glm::vec2 basisB;

        glm::u16vec2 uvTop;
        glm::u16vec2 uvBottom;

        uint16_t texture;
        uint16_t mask;

        glm::u8vec3 color;

        LayerType type;
    };
#pragma pack( pop )

    enum class SelectionFlags : uint32_t
    {
        InsideBox  = 1 << 0,
        OutsideBox = 1 << 1
    };

#pragma pack( push )
    struct Selection
    {
        glm::vec4 bbox;
        SelectionFlags flags;
    };
#pragma pack( pop )

    class Layers
    {
      public:
        Layers( size_t maxLayers );
        ~Layers() = default;

        bool add( const Layer& layer );
        bool move( int to, int from );
        bool remove( int index );

        size_t length() const;
        Layer* data() const;

      private:
        size_t m_maxLength;
        size_t m_curLength;

        std::unique_ptr<Layer[]> m_array;
    };
} // namespace mc
