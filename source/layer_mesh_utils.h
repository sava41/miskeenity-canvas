#pragma once

#include "layer_manager.h"
#include "mesh_manager.h"

#include <string>

namespace mc
{
    void addGlyphLayers( LayerManager& layerManager, MeshManager& meshManager, int startLayer, const std::string& string, const glm::vec2& position,
                         int atlasTextureId, const glm::vec3& color, float outline, const glm::vec3& outlineColor );
} // namespace mc