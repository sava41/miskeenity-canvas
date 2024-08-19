#pragma once

#include <memory>
#include <vector>
#include <webgpu/webgpu_cpp.h>

namespace mc
{
    // we need to store all our meshes in one array since webgpu doesnt have bind arrays right now.
    // for now only grow this array since our meshes are relatively small
    constexpr int UnitSquareMeshIndex = 0;

    struct MeshInfo
    {
        uint16_t start;
        uint16_t length;
    };

#pragma pack( push, 16 )
    struct Vertex
    {
        float x;
        float y;
        float u;
        float v;
        float sizex;
        float sizey;
        uint32_t color;
        uint32_t pad;
    };
#pragma pack( pop )
    static_assert( sizeof( Vertex ) % 16 == 0 );

#pragma pack( push )
    struct Triangle
    {
        Vertex v1;
        Vertex v2;
        Vertex v3;
    };
#pragma pack( pop )

    class MeshManager
    {
      public:
        MeshManager( size_t maxLength );
        ~MeshManager() = default;

        bool add( const std::vector<Triangle>& meshBuffer );
        bool add( const Triangle* meshBuffer, size_t size );

        size_t size() const;
        size_t numTriangles() const;
        size_t numMeshes() const;
        size_t maxLength() const;

        MeshInfo getMeshInfo( int index ) const;
        Triangle* data() const;

        mc::MeshManager& operator=( const mc::MeshManager& );

      private:
        size_t m_length;
        size_t m_maxLength;

        std::unique_ptr<Triangle[]> m_meshArray;
        std::vector<MeshInfo> m_meshInfoArray;
    };
} // namespace mc
