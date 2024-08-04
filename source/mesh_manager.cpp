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

    void MeshManager::add( const std::vector<Triangle>& meshBuffer )
    {
        if( m_length + meshBuffer.size() > MaxMeshBufferTriangles )
        {
            return;
        }

        m_meshInfoArray.push_back( { static_cast<uint16_t>( m_length ), static_cast<uint16_t>( meshBuffer.size() ) } );

        size_t newLength                         = m_length + meshBuffer.size();
        std::unique_ptr<Triangle[]> newMeshArray = std::make_unique<Triangle[]>( newLength );

        std::memcpy( newMeshArray.get(), m_meshArray.get(), m_length * sizeof( Triangle ) );
        m_meshArray = std::move( newMeshArray );

        std::memcpy( m_meshArray.get() + m_length * sizeof( Triangle ), meshBuffer.data(), meshBuffer.size() * sizeof( Triangle ) );

        m_length = newLength;
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
