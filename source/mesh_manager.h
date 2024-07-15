#pragma once

#include <memory>
#include <vector>
#include <webgpu/webgpu_cpp.h>

namespace mc
{
    // we need to store all our meshes in one array since webgpu doesnt have bind arrays right now.
    // for now only grow this array since our meshes are relatively small

    struct MeshInfo
    {
        size_t start;
        size_t length;
    };

    struct Vertex
    {
        float x;
        float y;
        float u;
        float v;
    };

    struct Triangle
    {
        Vertex v1;
        Vertex v2;
        Vertex v3;
    };

    class MeshManager
    {
      public:
        MeshManager();
        ~MeshManager() = default;

        void add( const std::vector<Triangle>& meshBuffer );

        size_t length() const;
        size_t numMeshes() const;
        MeshInfo getMeshInfo( int index ) const;
        Triangle* data() const;

      private:
        size_t m_length;

        std::unique_ptr<Triangle[]> m_meshArray;
        std::vector<MeshInfo> m_meshInfoArray;
    };
} // namespace mc
