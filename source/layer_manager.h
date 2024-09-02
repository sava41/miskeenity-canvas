#pragma once

#include "resource_manager.h"

#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

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

        glm::u8vec4 color;
        uint32_t flags;

        uint16_t vertexBuffOffset;
        uint16_t vertexBuffLength;

        uint16_t texture;
        uint16_t mask;
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
        bool add( const Layer& layer, const ResourceHandle& textureHandle );
        bool move( int to, int from );
        bool remove( int index );
        void removeTop( int newlength );

        size_t length() const;
        size_t getTotalTriCount() const;
        Layer* data() const;
        ResourceHandle getTexture( int index );

        void changeSelection( int index, bool isSelected );
        void clearSelection();
        bool isSelected( int index ) const;
        size_t numSelected() const;

        void moveSelection( const glm::vec2& offset );
        void rotateSelection( const glm::vec2& center, float angle );
        void scaleSelection( const glm::vec2& center, const glm::vec2& ammount );
        void bringFrontSelection( bool reverse = false );
        void removeSelection();

      private:
        void recalculateTriCount();

        size_t m_maxLength;
        size_t m_curLength;
        size_t m_numSelected;
        size_t m_totalNumTri;

        std::unique_ptr<Layer[]> m_array;
        std::unordered_map<int, std::vector<ResourceHandle>> m_textureHandles;
    };
} // namespace mc
