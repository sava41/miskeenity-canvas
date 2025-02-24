#pragma once

#include "resource_manager.h"

#include <glm/glm.hpp>
#include <limits>
#include <memory>
#include <unordered_map>

namespace mc
{

    const int UV_MAX_VALUE = std::numeric_limits<uint16_t>::max();

    enum LayerFlags : uint32_t
    {
        Selected        = 1 << 0,
        HasColorTex     = 1 << 1,
        HasMaskTex      = 1 << 2,
        HasSdfMaskTex   = 1 << 3,
        HasPillAlphaTex = 1 << 4,
        InvertMask      = 1 << 5
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

        uint16_t texture = 0;
        uint16_t mask    = 0;

        // some layers may need addional paramters so well store them here
        union
        {
            uint32_t extra0 = 0;
            glm::u8vec4 outlineColor;
        };

        union
        {
            uint32_t extra1 = 0;
            float outlineWidth;
        };

        union
        {
            uint32_t extra2 = 0;
            float fontSize;
        };

        uint32_t extra3 = 0;
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

    struct MeshInfo;

    class LayerManager
    {
      public:
        LayerManager( size_t maxLayers );
        LayerManager();
        ~LayerManager() = default;

        LayerManager( LayerManager& source );
        LayerManager( LayerManager&& source );
        LayerManager& operator=( LayerManager& );
        LayerManager& operator=( LayerManager&& );

        bool add( const Layer& layer, const ResourceHandle& textureHandle = ResourceHandle::invalidResource(),
                  const ResourceHandle& maskHandle = ResourceHandle::invalidResource() );
        bool add( glm::vec2 offset, glm::vec2 basisA, glm::vec2 basisB, glm::u16vec2 uvTop, glm::u16vec2 uvBottom, glm::u8vec4 color, uint32_t flags,
                  MeshInfo meshInfo, const ResourceHandle& textureHandle = ResourceHandle::invalidResource(),
                  const ResourceHandle& maskHandle = ResourceHandle::invalidResource() );
        bool move( int to, int from );
        bool remove( int index );
        void removeTop( int newlength );

        size_t length() const;
        size_t getTotalTriCount() const;
        Layer* data() const;
        Layer getUncroppedLayer( int index ) const;
        const ResourceHandle& getTexture( int index ) const;
        const ResourceHandle& getMask( int index ) const;

        void changeSelection( int index, bool isSelected );
        void clearSelection();
        bool isSelected( int index ) const;
        int getSingleSelectedImage() const;
        size_t numSelected() const;

        void translateSelection( const glm::vec2& offset );
        void rotateSelection( const glm::vec2& center, float angle );
        void scaleSelection( const glm::vec2& center, const glm::vec2& ammount );
        void bringFrontSelection( bool reverse = false );
        void duplicateSelection( const glm::vec2& offset );
        void removeSelection();

        LayerManager createShrunkCopy();

        // this will keep the array allocation the same, copy contents and handle cases where source array size != array size
        void copyContents( const LayerManager& source );

      private:
        void recalculateTriCount();

        size_t m_maxLength;
        size_t m_curLength;
        size_t m_numSelected;
        size_t m_totalNumTri;

        std::unique_ptr<Layer[]> m_array;
        std::unordered_map<int, ResourceHandle> m_textureHandles;
        // keep internal counter of texture usage so we dont have to store multiple texture handles;
        std::unordered_map<int, int> m_textureReferences;
    };
} // namespace mc
