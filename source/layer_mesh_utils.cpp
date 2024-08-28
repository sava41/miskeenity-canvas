#include "layer_mesh_utils.h"

#include <glm/glm.hpp>

namespace mc
{
    void addGlyphLayers( LayerManager& layerManager, MeshManager& meshManager, int startLayer, const std::string& string, const glm::vec2& position,
                         int atlasTextureId, const glm::vec3& textColor, float outline, const glm::vec3& outlineColor )
    {
        glm::vec2 currentPosition = position;

        layerManager.removeTop( startLayer );

        for( const char c : string )
        {
            if( c == '\n' )
            {
                currentPosition.x = position.x;
                currentPosition.y += 100.0;
            }
            else if( c == ' ' )
            {
                currentPosition.x += 100.0;
            }
            else
            {
                glm::vec2 basisA      = glm::vec2( 0.0, 1.0 ) * 90.0f;
                glm::vec2 basisB      = glm::vec2( 1.0, 0.0 ) * 90.0f;
                glm::u8vec4 color     = glm::u8vec4( textColor * 255.0f, 255 );
                mc::MeshInfo meshInfo = meshManager.getMeshInfo( mc::UnitSquareMeshIndex );

                layerManager.add(
                    { currentPosition, basisA, basisB, glm::u16vec2( 0 ), glm::u16vec2( 1.0 ), color, 0, meshInfo.start, meshInfo.length, 0, 0 } );

                currentPosition.x += 100.0;
            }
        }
    }
} // namespace mc