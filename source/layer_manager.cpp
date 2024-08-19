#include "layer_manager.h"

#include <SDL3/SDL.h>

namespace mc
{

    LayerManager::LayerManager( size_t maxLayers )
        : m_curLength( 0 )
        , m_maxLength( maxLayers )
        , m_array( std::make_unique<Layer[]>( maxLayers ) )
    {
    }

    bool LayerManager::add( const Layer& layer )
    {
        if( m_curLength == m_maxLength )
        {
            return false;
        }

        m_array[m_curLength] = layer;
        m_curLength += 1;

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

    size_t LayerManager::numSelected() const
    {
        return m_numSelected;
    }

    void LayerManager::moveSelection( const glm::vec2& offset )
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

    void LayerManager::removeSelection()
    {
        if( m_numSelected == 0 )
        {
            return;
        }

        size_t writeIndex = 0;
        for( size_t readIndex = 0; readIndex < m_curLength; ++readIndex )
        {
            if( !isSelected( readIndex ) )
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

} // namespace mc