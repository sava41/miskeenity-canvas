#pragma once

#include "mesh_manager.h"

namespace mc
{

    MeshManager::MeshManager()
        : m_length( 0 )
    {
        // add unit square mesh
        add( { { { -0.5, -0.5, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                 { +0.5, -0.5, 1.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                 { +0.5, +0.5, 1.0, 1.0, 1.0, 1.0, 0xFFFFFFFF, 0 } },
               { { -0.5, -0.5, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                 { +0.5, +0.5, 1.0, 1.0, 1.0, 1.0, 0xFFFFFFFF, 0 },
                 { -0.5, +0.5, 0.0, 1.0, 1.0, 1.0, 0xFFFFFFFF, 0 } } } );
    }

    bool MeshManager::add( const Triangle* meshBuffer, size_t length )
    {
        size_t newLength = m_length + length;

        if( newLength > MaxMeshBufferTriangles )
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

    MeshInfo MeshManager::getMeshInfo( int index ) const
    {
        return m_meshInfoArray[index];
    }

    Triangle* MeshManager::data() const
    {
        return m_meshArray.get();
    }

} // namespace mc
