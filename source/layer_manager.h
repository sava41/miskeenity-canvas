#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>

namespace mc
{

    enum LayerFlags : uint32_t
    {
        Selected        = 1 << 0,
        HasColorTex     = 1 << 1,
        HasAlphaTex     = 1 << 2,
        HasSdfAlphaTex  = 1 << 3,
        HasPillAlphaTex = 1 << 4
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

        glm::u8vec4 color;

        uint32_t flags;
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

    class LayerManager
    {
      public:
        LayerManager( size_t maxLayers );
        ~LayerManager() = default;

        bool add( const Layer& layer );
        bool move( int to, int from );
        bool remove( int index );

        size_t length() const;
        Layer* data() const;

        void changeSelection( int index, bool isSelected );
        void clearSelection();
        bool isSelected( int index ) const;
        size_t numSelected() const;

        void moveSelection( const glm::vec2& offset );
        void rotateSelection( const glm::vec2& center, float angle );
        void scaleSelection( const glm::vec2& center, const glm::vec2& ammount );

      private:
        size_t m_maxLength;
        size_t m_curLength;
        size_t m_numSelected;

        std::unique_ptr<Layer[]> m_array;
    };
} // namespace mc
