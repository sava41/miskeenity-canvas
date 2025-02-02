#include "layer_manager.h"

#include "mesh_manager.h"

#include <SDL3/SDL.h>

namespace mc
{

    LayerManager::LayerManager( size_t maxLayers )
        : m_curLength( 0 )
        , m_maxLength( maxLayers )
        , m_array( std::make_unique<Layer[]>( maxLayers ) )
    {
    }

    LayerManager::LayerManager()
        : m_curLength( 0 )
        , m_maxLength( 0 )
        , m_array( std::make_unique<Layer[]>( 0 ) )
    {
    }

    LayerManager::LayerManager( LayerManager& source )
        : m_curLength( source.m_curLength )
        , m_maxLength( source.m_maxLength )
        , m_numSelected( source.m_numSelected )
        , m_totalNumTri( source.m_totalNumTri )
        , m_array( std::make_unique<Layer[]>( source.m_maxLength ) )
        , m_textureHandles( source.m_textureHandles )
        , m_textureReferences( source.m_textureReferences )
    {
        std::memcpy( m_array.get(), source.m_array.get(), source.m_curLength * sizeof( Layer ) );
    }

    LayerManager::LayerManager( LayerManager&& source )
        : m_curLength( source.m_curLength )
        , m_maxLength( source.m_maxLength )
        , m_numSelected( source.m_numSelected )
        , m_totalNumTri( source.m_totalNumTri )
        , m_array( std::move( source.m_array ) )
        , m_textureHandles( std::move( source.m_textureHandles ) )
        , m_textureReferences( source.m_textureReferences )
    {
        source.m_curLength   = 0;
        source.m_maxLength   = 0;
        source.m_numSelected = 0;
        source.m_totalNumTri = 0;
    }

    LayerManager& LayerManager::operator=( LayerManager& source )
    {
        m_curLength         = source.m_curLength;
        m_maxLength         = source.m_maxLength;
        m_numSelected       = source.m_numSelected;
        m_totalNumTri       = source.m_totalNumTri;
        m_textureReferences = source.m_textureReferences;

        m_array = std::make_unique<Layer[]>( m_maxLength );
        std::memcpy( m_array.get(), source.m_array.get(), source.m_curLength * sizeof( Layer ) );

        m_textureHandles.clear();
        m_textureHandles.insert( source.m_textureHandles.begin(), source.m_textureHandles.end() );

        return *this;
    }
    LayerManager& LayerManager::operator=( LayerManager&& source )
    {
        m_curLength         = source.m_curLength;
        m_maxLength         = source.m_maxLength;
        m_numSelected       = source.m_numSelected;
        m_totalNumTri       = source.m_totalNumTri;
        m_textureReferences = source.m_textureReferences;

        m_array          = std::move( source.m_array );
        m_textureHandles = std::move( source.m_textureHandles );

        source.m_curLength   = 0;
        source.m_maxLength   = 0;
        source.m_numSelected = 0;
        source.m_totalNumTri = 0;

        return *this;
    }

    bool LayerManager::add( glm::vec2 offset, glm::vec2 basisA, glm::vec2 basisB, glm::u16vec2 uvTop, glm::u16vec2 uvBottom, glm::u8vec4 color, uint32_t flags,
                            MeshInfo meshInfo, const ResourceHandle& textureHandle, const ResourceHandle& maskHandle )
    {
        uint16_t textureIndex = flags & LayerFlags::HasColorTex ? static_cast<uint16_t>( textureHandle.resourceIndex() ) : 0;
        uint16_t maskIndex    = flags & LayerFlags::HasMaskTex || flags & LayerFlags::HasSdfMaskTex ? static_cast<uint16_t>( maskHandle.resourceIndex() ) : 0;
        Layer layer           = { offset, basisA, basisB, uvTop, uvBottom, color, flags, meshInfo.start, meshInfo.length, textureIndex, maskIndex };

        return add( layer, textureHandle, maskHandle );
    }

    bool LayerManager::add( const Layer& layer, const ResourceHandle& textureHandle, const ResourceHandle& maskHandle )
    {
        if( m_curLength == m_maxLength )
        {
            return false;
        }

        m_array[m_curLength] = layer;

        if( layer.flags & LayerFlags::HasColorTex && textureHandle.valid() )
        {
            if( !m_textureReferences.contains( textureHandle.resourceIndex() ) || m_textureReferences[textureHandle.resourceIndex()] == 0 )
            {
                m_textureHandles.insert( { textureHandle.resourceIndex(), textureHandle } );
            }
            m_textureReferences[textureHandle.resourceIndex()] += 1;
            m_array[m_curLength].texture = textureHandle.resourceIndex();
        }

        if( ( layer.flags & LayerFlags::HasMaskTex || layer.flags & LayerFlags::HasSdfMaskTex ) && maskHandle.valid() )
        {
            if( !m_textureReferences.contains( maskHandle.resourceIndex() ) || m_textureReferences[maskHandle.resourceIndex()] == 0 )
            {
                m_textureHandles.insert( { maskHandle.resourceIndex(), maskHandle } );
            }
            m_textureReferences[maskHandle.resourceIndex()] += 1;
            m_array[m_curLength].mask = maskHandle.resourceIndex();
        }

        m_curLength += 1;

        if( layer.flags & LayerFlags::Selected )
        {
            m_numSelected += 1;
        }

        recalculateTriCount();

        return true;
    }

    bool LayerManager::move( int to, int from )
    {
        if( to < 0 || from < 0 || to > m_curLength || from > m_curLength || to == from )
        {
            return false;
        }

        Layer temp = m_array[from];

        if( to > from )
        {
            std::memmove( m_array.get() + from, m_array.get() + from + 1, ( to - from ) * sizeof( Layer ) );
        }
        else
        {
            std::memmove( m_array.get() + to + 1, m_array.get() + to, ( from - to ) * sizeof( Layer ) );
        }

        m_array[to] = temp;

        return true;
    }

    bool LayerManager::remove( int index )
    {
        if( index < 0 || index >= m_curLength )
        {
            return false;
        }

        if( m_array[index].flags & LayerFlags::Selected )
        {
            m_numSelected -= 1;
        }

        if( m_array[index].flags & LayerFlags::HasColorTex )
        {
            m_textureReferences[m_array[index].texture] -= 1;

            if( m_textureReferences[m_array[index].texture] == 0 )
            {
                m_textureHandles.erase( m_array[index].texture );
            }
        }

        if( m_array[index].flags & LayerFlags::HasMaskTex || m_array[index].flags & LayerFlags::HasSdfMaskTex )
        {
            m_textureReferences[m_array[index].mask] -= 1;

            if( m_textureReferences[m_array[index].mask] == 0 )
            {
                m_textureHandles.erase( m_array[index].mask );
            }
        }

        if( index != m_curLength - 1 )
        {
            std::memmove( m_array.get() + index, m_array.get() + index + 1, ( m_curLength - index - 1 ) * sizeof( Layer ) );
        }

        m_curLength -= 1;

        recalculateTriCount();

        return true;
    }

    void LayerManager::removeTop( int newLength )
    {
        if( m_curLength > newLength && newLength >= 0 )
        {

            for( int i = newLength; i < m_curLength; ++i )
            {
                if( m_array[i].flags & LayerFlags::Selected )
                {
                    m_numSelected -= 1;
                }

                if( m_array[i].flags & LayerFlags::HasColorTex )
                {
                    m_textureReferences[m_array[i].texture] -= 1;

                    if( m_textureReferences[m_array[i].texture] == 0 )
                    {
                        m_textureHandles.erase( m_array[i].texture );
                    }
                }

                if( m_array[i].flags & LayerFlags::HasMaskTex || m_array[i].flags & LayerFlags::HasSdfMaskTex )
                {
                    m_textureReferences[m_array[i].mask] -= 1;

                    if( m_textureReferences[m_array[i].mask] == 0 )
                    {
                        m_textureHandles.erase( m_array[i].mask );
                    }
                }
            }

            m_curLength = newLength;

            recalculateTriCount();
        }
    }

    size_t LayerManager::length() const
    {
        return m_curLength;
    }

    size_t LayerManager::getTotalTriCount() const
    {
        return m_totalNumTri;
    }

    Layer* LayerManager::data() const
    {
        return m_array.get();
    }

    Layer LayerManager::getUncroppedLayer( int index ) const
    {
        if( index < 0 || index >= m_curLength )
        {
            return {};
        }

        Layer uncroppedLayer = m_array[index];

        glm::vec2 scale = static_cast<float>( mc::UV_MAX_VALUE ) / glm::vec2( uncroppedLayer.uvBottom - uncroppedLayer.uvTop );
        uncroppedLayer.basisA *= scale.x;
        uncroppedLayer.basisB *= scale.y;

        glm::vec2 uvCenter = ( glm::vec2( uncroppedLayer.uvTop ) + glm::vec2( uncroppedLayer.uvBottom ) ) / static_cast<float>( mc::UV_MAX_VALUE ) * 0.5f;
        uncroppedLayer.offset -= uncroppedLayer.basisA * ( uvCenter.x - 0.5f ) + uncroppedLayer.basisB * ( uvCenter.y - 0.5f );

        uncroppedLayer.uvTop    = glm::u16vec2( 0 );
        uncroppedLayer.uvBottom = glm::u16vec2( mc::UV_MAX_VALUE );

        return uncroppedLayer;
    }

    void LayerManager::changeSelection( int index, bool isSelected )
    {
        if( index < 0 || index >= m_curLength )
        {
            return;
        }

        if( isSelected && !( m_array[index].flags & LayerFlags::Selected ) )
        {
            m_array[index].flags = m_array[index].flags | LayerFlags::Selected;
            m_numSelected += 1;
        }

        if( !isSelected && ( m_array[index].flags & LayerFlags::Selected ) )
        {
            m_array[index].flags = m_array[index].flags & ~LayerFlags::Selected;
            m_numSelected -= 1;
        }
    }

    void LayerManager::clearSelection()
    {
        for( int i = 0; i < m_curLength; ++i )
        {
            changeSelection( i, false );
        }
    }

    bool LayerManager::isSelected( int index ) const
    {
        if( index < 0 || index >= m_curLength )
        {
            return false;
        }

        return m_array[index].flags & LayerFlags::Selected;
    }

    int LayerManager::getSingleSelectedImage() const
    {
        if( m_numSelected != 1 )
        {
            return -1;
        }

        for( int i = 0; i < m_curLength; ++i )
        {
            if( m_array[i].flags & LayerFlags::Selected && m_array[i].flags & LayerFlags::HasColorTex )
            {
                return i;
            }
        }

        return -1;
    }

    size_t LayerManager::numSelected() const
    {
        return m_numSelected;
    }

    void LayerManager::translateSelection( const glm::vec2& offset )
    {
        for( int i = 0; i < m_curLength; ++i )
        {
            if( m_array[i].flags & LayerFlags::Selected )
            {
                m_array[i].offset += offset;
            }
        }
    }

    void LayerManager::rotateSelection( const glm::vec2& center, float angle )
    {
        float cos = std::cos( angle );
        float sin = std::sin( angle );

        for( int i = 0; i < m_curLength; ++i )
        {
            if( m_array[i].flags & LayerFlags::Selected )
            {
                m_array[i].basisA = glm::vec2( m_array[i].basisA.x * cos - m_array[i].basisA.y * sin, m_array[i].basisA.x * sin + m_array[i].basisA.y * cos );
                m_array[i].basisB = glm::vec2( m_array[i].basisB.x * cos - m_array[i].basisB.y * sin, m_array[i].basisB.x * sin + m_array[i].basisB.y * cos );


                m_array[i].offset -= center;
                m_array[i].offset = glm::vec2( m_array[i].offset.x * cos - m_array[i].offset.y * sin, m_array[i].offset.x * sin + m_array[i].offset.y * cos );
                m_array[i].offset += center;
            }
        }
    }

    void LayerManager::scaleSelection( const glm::vec2& center, const glm::vec2& ammount )
    {
        for( int i = 0; i < m_curLength; ++i )
        {
            if( m_array[i].flags & LayerFlags::Selected )
            {
                m_array[i].basisA *= ammount;
                m_array[i].basisB *= ammount;

                m_array[i].offset -= center;
                m_array[i].offset *= ammount;
                m_array[i].offset += center;
            }

            // special case for text layers
            if( m_array[i].flags & LayerFlags::HasSdfMaskTex )
            {
                m_array[i].fontSize *= ( std::abs( ammount.x ) + std::abs( ammount.y ) ) * 0.5;
            }
        }
    }

    void LayerManager::bringFrontSelection( bool reverse )
    {
        if( m_numSelected == 0 || m_numSelected == m_curLength )
        {
            return;
        }

        size_t unselectedIndex = reverse ? m_numSelected : 0;
        size_t selectedIndex   = reverse ? 0 : m_curLength - m_numSelected;

        std::unique_ptr<Layer[]> newArray = std::make_unique<Layer[]>( m_maxLength );

        for( int i = 0; i < m_curLength; ++i )
        {
            if( isSelected( i ) )
            {
                newArray[selectedIndex++] = m_array[i];
            }
            else
            {
                newArray[unselectedIndex++] = m_array[i];
            }
        }

        m_array = std::move( newArray );
    }

    void LayerManager::duplicateSelection( const glm::vec2& offset )
    {
        int length = m_curLength;

        for( int i = 0; i < length; ++i )
        {
            if( m_array[i].flags & LayerFlags::Selected )
            {
                Layer duplicateLayer = m_array[i];
                duplicateLayer.offset += offset;

                ResourceHandle texture =
                    m_array[i].flags & LayerFlags::HasColorTex ? m_textureHandles.at( m_array[i].texture ) : ResourceHandle::invalidResource();
                ResourceHandle mask = m_array[i].flags & LayerFlags::HasMaskTex || m_array[i].flags & LayerFlags::HasSdfMaskTex
                                          ? m_textureHandles.at( m_array[i].mask )
                                          : ResourceHandle::invalidResource();

                add( duplicateLayer, texture, mask );
            }
        }
    }

    void LayerManager::removeSelection()
    {
        if( m_numSelected == 0 )
        {
            return;
        }

        size_t writeIndex = 0;

        for( size_t readIndex = 0; readIndex < m_curLength; ++readIndex )
        {
            if( isSelected( readIndex ) )
            {
                if( m_array[readIndex].flags & LayerFlags::HasColorTex )
                {
                    m_textureReferences[m_array[readIndex].texture] -= 1;

                    if( m_textureReferences[m_array[readIndex].texture] == 0 )
                    {
                        m_textureHandles.erase( m_array[readIndex].texture );
                    }
                }

                if( m_array[readIndex].flags & LayerFlags::HasMaskTex || m_array[readIndex].flags & LayerFlags::HasSdfMaskTex )
                {
                    m_textureReferences[m_array[readIndex].mask] -= 1;

                    if( m_textureReferences[m_array[readIndex].mask] == 0 )
                    {
                        m_textureHandles.erase( m_array[readIndex].mask );
                    }
                }
            }
            else
            {
                if( writeIndex != readIndex )
                {
                    m_array[writeIndex] = m_array[readIndex];
                }
                ++writeIndex;
            }
        }

        m_curLength -= m_numSelected;
        m_numSelected = 0;

        recalculateTriCount();
    }

    void LayerManager::recalculateTriCount()
    {
        size_t count = 0;

        for( int i = 0; i < m_curLength; ++i )
        {
            count += m_array[i].vertexBuffLength;
        }

        m_totalNumTri = count;
    }

    const ResourceHandle& LayerManager::getTexture( int index ) const
    {
        // were using an invalid resource handle for layers with no textures
        if( ( index < 0 || index >= m_curLength ) || !( m_array[index].flags & LayerFlags::HasColorTex ) )
        {
            return ResourceHandle::invalidResource();
        }

        return m_textureHandles.at( m_array[index].texture );
    }

    const ResourceHandle& LayerManager::getMask( int index ) const
    {
        // were using an invalid resource handle for layers with no textures
        if( ( index < 0 || index >= m_curLength ) || !( m_array[index].flags & LayerFlags::HasMaskTex || m_array[index].flags & LayerFlags::HasSdfMaskTex ) )
        {
            return ResourceHandle::invalidResource();
        }

        return m_textureHandles.at( m_array[index].mask );
    }

    LayerManager LayerManager::createShrunkCopy()
    {
        LayerManager newManager( m_curLength );

        std::memcpy( newManager.m_array.get(), m_array.get(), m_curLength * sizeof( Layer ) );

        newManager.m_curLength         = m_curLength;
        newManager.m_numSelected       = m_numSelected;
        newManager.m_totalNumTri       = m_totalNumTri;
        newManager.m_textureReferences = m_textureReferences;
        newManager.m_textureHandles.insert( m_textureHandles.begin(), m_textureHandles.end() );

        return std::move( newManager );
    }

    void LayerManager::copyContents( const LayerManager& source )
    {
        m_curLength = std::min( m_maxLength, source.m_curLength );
        std::memcpy( m_array.get(), source.m_array.get(), m_curLength * sizeof( Layer ) );

        m_numSelected = 0;
        for( int i = 0; i < m_curLength; ++i )
        {
            m_numSelected += m_array[i].flags & LayerFlags::Selected;
        }

        m_textureReferences = source.m_textureReferences;
        m_textureHandles.clear();
        m_textureHandles.insert( source.m_textureHandles.begin(), source.m_textureHandles.end() );
        for( int i = m_curLength; i < source.m_curLength; ++i )
        {
            if( m_array[i].flags & LayerFlags::HasColorTex )
            {
                m_textureReferences[m_array[i].texture] -= 1;

                if( m_textureReferences[m_array[i].texture] == 0 )
                {
                    m_textureHandles.erase( m_array[i].texture );
                }
            }

            if( m_array[i].flags & LayerFlags::HasMaskTex || m_array[i].flags & LayerFlags::HasSdfMaskTex )
            {
                m_textureReferences[m_array[i].mask] -= 1;

                if( m_textureReferences[m_array[i].mask] == 0 )
                {
                    m_textureHandles.erase( m_array[i].mask );
                }
            }
        }

        recalculateTriCount();
    }

} // namespace mc