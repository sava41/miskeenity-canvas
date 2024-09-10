#pragma once

#include "layer_manager.h"
#include "mesh_manager.h"
#include "texture_manager.h"

#include <array>
#include <string>
#include <vector>

namespace mc
{

    struct Glyph
    {
        int x;
        int y;
        int width;
        int height;
        int xOffset;
        int yOffset;
        float xAdvance;
        float leftSideBearing;
    };

    class FontManager
    {

      public:
        FontManager();
        ~FontManager();

        enum Font : size_t
        {
            Arial = 0,
            TimesNewRoman,
            Impact,
            NumFonts
        };

        enum Alignment
        {
            Left = 0,
            Right,
            Center
        };

        void init( TextureManager& textureManager, const wgpu::Device& device, const MeshInfo& unitsquareMesh );

        void buildText( const std::string& string, Font font, LayerManager& layerManager, Alignment alignment, const glm::vec2& position, float scale,
                        const glm::vec3& textColor, float outline, const glm::vec3& outlineColor );


      private:
        MeshInfo m_unitSquareMesh;

        // for now well only store the basic ascii latin characters
        std::vector<std::array<Glyph, 128>> m_characterData;
        std::vector<ResourceHandle> m_fontTextures;
    };
} // namespace mc