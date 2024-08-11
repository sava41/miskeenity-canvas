#include "layer_mesh_utils.h"

#include <glm/glm.hpp>

namespace mc
{
    void removeTopLayers( LayerManager& layerManager, int newLength )
    {
        for( int i = layerManager.length() - 1; i >= newLength; --i )
        {
            layerManager.remove( i );
        }
    }

} // namespace mc