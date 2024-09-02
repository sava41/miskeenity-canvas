#include "resource_manager.h"


namespace mc
{

    ResourceHandle::ResourceHandle( ResourceManager* manager, int index )
        : m_manager( manager )
        , m_resourceIndex( index )
        , m_valid( index != -1 )
    {
        if( m_valid )
        {
            m_manager->m_references[m_resourceIndex] += 1;
            m_manager->m_handles.insert( this );
        }
    }

    ResourceHandle::~ResourceHandle()
    {
        if( m_valid )
        {
            m_manager->m_references[m_resourceIndex] -= 1;
            m_manager->m_handles.erase( this );
            if( m_manager->m_references[m_resourceIndex] == 0 )
            {
                m_manager->freeResource( m_resourceIndex );
                m_manager->m_length -= 1;
            }
        }
    }

    ResourceHandle ResourceHandle::invalidResource()
    {
        return ResourceHandle( nullptr, -1 );
    }

    ResourceHandle::ResourceHandle( const ResourceHandle& handle )
        : m_manager( handle.m_manager )
        , m_resourceIndex( handle.m_resourceIndex )
        , m_valid( handle.m_valid )
    {
        if( m_valid )
        {
            m_manager->m_references[m_resourceIndex] += 1;
            m_manager->m_handles.insert( this );
        }
    }

    ResourceHandle::ResourceHandle( ResourceHandle&& handle )
        : m_manager( handle.m_manager )
        , m_resourceIndex( handle.m_resourceIndex )
        , m_valid( handle.m_valid )
    {
        if( m_valid )
        {
            m_manager->m_handles.erase( &handle );
            m_manager->m_handles.insert( this );
        }
    }

    bool ResourceHandle::valid() const
    {
        return m_valid;
    }

    int ResourceHandle::resourceIndex() const
    {
        return m_resourceIndex;
    }

    ResourceManager::ResourceManager( size_t maxLength )
        : m_maxLength( maxLength )
        , m_length( 0 )
        , m_references( std::make_unique<int[]>( maxLength ) )
    {
        for( int i = 0; i < m_maxLength; ++i )
        {
            m_references[i] = 0;
        }
    }

    ResourceManager::~ResourceManager()
    {
        for( ResourceHandle* handle : m_handles )
        {
            handle->m_valid   = false;
            handle->m_manager = nullptr;
        }
    }

    size_t ResourceManager::maxLength() const
    {
        return m_maxLength;
    }

    size_t ResourceManager::curLength() const
    {
        return m_length;
    }

    ResourceHandle ResourceManager::getHandle( int resourceIndex )
    {
        if( resourceIndex < 0 || resourceIndex >= m_maxLength )
        {
            return ResourceHandle( this, -1 );
        }

        return ResourceHandle( this, resourceIndex );
    }

    int ResourceManager::getRefCount( int resourceIndex )
    {
        if( resourceIndex < 0 || resourceIndex >= m_maxLength )
        {
            return -1;
        }

        return m_references[resourceIndex];
    }
} // namespace mc