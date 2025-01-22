#include "layer_history.h"

namespace mc
{

    LayerHistory::LayerHistory( size_t maxMementos )
        : m_maxLength( maxMementos )
        , m_mementos( std::make_unique<LayerManager[]>( maxMementos ) )
        , m_currentMemento( 0 )
        , m_front( 1 )
        , m_back( 0 )
        , m_full( false )
        , m_checkpoint( -1 )
    {
    }


    void LayerHistory::push( LayerManager&& memento )
    {
        m_currentMemento = ( m_currentMemento + 1 ) % m_maxLength;
        m_front          = ( m_currentMemento + 1 ) % m_maxLength;

        m_mementos[m_currentMemento] = memento;

        m_full = m_front == m_back;

        if( m_full )
        {
            m_back = ( m_back + 1 ) % m_maxLength;
        }
    }

    const LayerManager& LayerHistory::undo()
    {
        if( m_back != m_currentMemento )
        {
            m_currentMemento = ( m_currentMemento - 1 ) % m_maxLength;
        }

        return m_mementos[m_currentMemento];
    }

    const LayerManager& LayerHistory::redo()
    {
        if( m_front != ( m_currentMemento + 1 ) % m_maxLength )
        {
            m_currentMemento = ( m_currentMemento + 1 ) % m_maxLength;
        }

        return m_mementos[m_currentMemento];
    }

    bool LayerHistory::atFront() const
    {
        return m_front == ( m_currentMemento + 1 ) % m_maxLength;
    }

    bool LayerHistory::atBack() const
    {
        if( m_checkpoint != -1 )
        {
            return m_checkpoint == m_currentMemento;
        }

        return m_back == m_currentMemento;
    }

    void LayerHistory::setCheckpoint()
    {
        m_checkpoint = m_currentMemento;
        m_front      = ( m_currentMemento + 1 ) % m_maxLength;
    }

    void LayerHistory::removeCheckpoint()
    {
        m_checkpoint = -1;
    }

    const LayerManager& LayerHistory::resetToCheckpoint()
    {
        if( m_checkpoint != -1 )
        {
            m_currentMemento = m_checkpoint;
            m_front          = ( m_currentMemento + 1 ) % m_maxLength;

            m_checkpoint = -1;
        }

        return m_mementos[m_currentMemento];
    }

    size_t LayerHistory::length() const
    {
        if( m_full )
        {
            return m_maxLength;
        }

        if( m_front >= m_back )
        {
            return m_front - m_back;
        }

        return m_maxLength - m_back + m_front;
    }

} // namespace mc