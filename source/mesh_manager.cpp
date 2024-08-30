#include "mesh_manager.h"

namespace mc
{

    MeshManager::MeshManager( size_t maxLength )
        : m_length( 0 )
        , m_maxLength( maxLength )
    {
    }

    bool MeshManager::add( const Triangle* meshBuffer, size_t length )
    {
        size_t newLength = m_length + length;

        if( newLength > m_maxLength )
        {
            return false;
        }

        m_meshInfoArray.push_back( { static_cast<uint16_t>( m_length ), static_cast<uint16_t>( length ) } );

        std::unique_ptr<Triangle[]> newMeshArray = std::make_unique<Triangle[]>( newLength );

        std::memcpy( newMeshArray.get(), m_meshArray.get(), m_length * sizeof( Triangle ) );
        m_meshArray = std::move( newMeshArray );

        std::memcpy( m_meshArray.get() + m_length, meshBuffer, length * sizeof( Triangle ) );

        m_length = newLength;

        return true;
    }

    bool MeshManager::add( const std::vector<Triangle>& meshBuffer )
    {
        return add( meshBuffer.data(), meshBuffer.size() );
    }

    size_t MeshManager::size() const
    {
        return m_length * sizeof( Triangle );
    }

    size_t MeshManager::numTriangles() const
    {
        return m_length;
    }

    size_t MeshManager::numMeshes() const
    {
        return m_meshInfoArray.size();
    }

    size_t MeshManager::maxLength() const
    {
        return m_maxLength;
    }

    MeshInfo MeshManager::getMeshInfo( int index ) const
    {
        return m_meshInfoArray[index];
    }

    Triangle* MeshManager::data() const
    {
        return m_meshArray.get();
    }

    MeshManager& MeshManager::operator=( const MeshManager& other )
    {
        if( this != &other )
        {
            m_length    = other.m_length;
            m_maxLength = other.m_maxLength;

            m_meshArray = std::make_unique<Triangle[]>( m_maxLength );
            std::memcpy( m_meshArray.get(), other.m_meshArray.get(), m_length );

            // Copy the mesh info array
            m_meshInfoArray = other.m_meshInfoArray;
        }
        return *this;
    }

} // namespace mc
