#include "font_manager.h"

#include "app.h"
#include "battery/embed.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>


namespace mc
{

    const std::array<const char*, FontManager::NumFonts> FontSources = { b::embed<"./resources/fonts/Arimo_compact.ttf">().data(),
                                                                         b::embed<"./resources/fonts/EBGaramond_compact.ttf">().data(),
                                                                         b::embed<"./resources/fonts/Anton_compact.ttf">().data() };

    const int AtlasWidth   = 1024;
    const int Padding      = 12;
    const float GlyphScale = 128.0;

    FontManager::FontManager()
    {
    }
    FontManager::~FontManager()
    {
    }

    void FontManager::init( TextureManager& textureManager, const wgpu::Device& device, const MeshInfo& unitsquareMesh )
    {
        m_unitSquareMesh = unitsquareMesh;

        for( int j = 0; j < NumFonts; ++j )
        {
            m_characterData.push_back( {} );

            stbtt_fontinfo font;
            if( !stbtt_InitFont( &font, reinterpret_cast<const unsigned char*>( FontSources[j] ), 0 ) )
            {
                m_fontTextures.push_back( ResourceHandle::invalidResource() );
                continue;
            }

            float scale = stbtt_ScaleForPixelHeight( &font, GlyphScale );

            std::vector<stbrp_rect> rects;
            std::array<uint8_t*, 128> sdfBitmaps;

            for( int c = 0; c < 128; ++c )
            {
                int i = stbtt_FindGlyphIndex( &font, c );

                int advance, lsb;
                stbtt_GetGlyphHMetrics( &font, i, &advance, &lsb );

                Glyph glyph;

                glyph.xAdvance        = advance * scale;
                glyph.leftSideBearing = lsb * scale;

                if( stbtt_IsGlyphEmpty( &font, i ) )
                {
                    continue;
                }

                sdfBitmaps[c] = stbtt_GetGlyphSDF( &font, scale, i, Padding, 128, 16, &glyph.width, &glyph.height, &glyph.xOffset, &glyph.yOffset );

                rects.emplace_back( c, glyph.width, glyph.height, 0, 0, 0 );

                m_characterData.back()[c] = glyph;
            }

            // Pack glyph rectangles
            stbrp_context context;
            std::vector<stbrp_node> nodes( AtlasWidth );
            stbrp_init_target( &context, AtlasWidth, AtlasWidth, nodes.data(), nodes.size() );
            stbrp_pack_rects( &context, rects.data(), rects.size() );

            // Create atlas
            unsigned char* atlasData = new unsigned char[AtlasWidth * AtlasWidth];
            for( int i = 0; i < AtlasWidth * AtlasWidth; ++i )
            {
                atlasData[i] = 0;
            }

            for( size_t i = 0; i < rects.size(); ++i )
            {
                const stbrp_rect& rect = rects[i];

                if( rect.was_packed )
                {
                    Glyph& glyph = m_characterData.back()[rect.id];

                    glyph.x = rect.x;
                    glyph.y = rect.y;

                    // Copy glyph SDF to atlas
                    for( int y = 0; y < rect.h; ++y )
                    {
                        for( int x = 0; x < rect.w; ++x )
                        {
                            atlasData[( rect.y + y ) * AtlasWidth + ( rect.x + x )] = sdfBitmaps[rect.id][y * rect.w + x];
                        }
                    }
                }

                stbtt_FreeSDF( sdfBitmaps[rect.id], nullptr );
                sdfBitmaps[rect.id] = nullptr;
            }

            m_fontTextures.push_back( textureManager.add( atlasData, AtlasWidth, AtlasWidth, 1, device ) );

            device.GetQueue().OnSubmittedWorkDone( []( WGPUQueueWorkDoneStatus status, void* atlasData ) { delete atlasData; }, atlasData );
        }
    }

    void FontManager::buildText( const std::string& string, Font font, LayerManager& layerManager, Alignment alignment, const glm::vec2& position, float scale,
                                 const glm::vec3& textColor, float outline, const glm::vec3& outlineColor )
    {
        std::vector<float> lineWidths( 1, 0.0 );
        float width  = 0.0;
        float height = 115.0 * scale;

        for( const char c : string )
        {
            if( c == '\n' )
            {
                lineWidths.push_back( 0 );
                height += 100 * scale;
            }
            else if( c == ' ' )
            {
                lineWidths.back() += 100 * scale;
            }
            else
            {
                int codepoint = c;
                Glyph glyph   = m_characterData[font]['?'];
                if( codepoint < m_characterData[font].size() )
                {
                    glyph = m_characterData[font][codepoint];
                }

                lineWidths.back() += glyph.xAdvance * scale;
            }

            width = std::max( lineWidths.back(), width );
        }

        int line                  = 0;
        glm::vec2 currentPosition = position - glm::vec2( width, height ) * 0.5f;
        if( alignment == Alignment::Right )
        {
            currentPosition = position - glm::vec2( -width * 0.5f + lineWidths[line], height * 0.5f );
        }
        else if( alignment == Alignment::Center )
        {
            currentPosition = position - glm::vec2( lineWidths[line], height ) * 0.5f;
        }

        for( const char c : string )
        {
            if( c == '\n' )
            {
                line += 1;

                if( alignment == Alignment::Left )
                {
                    currentPosition.x = position.x - width * 0.5;
                }
                else if( alignment == Alignment::Right )
                {
                    currentPosition.x = position.x - ( -width * 0.5 + lineWidths[line] );
                }
                else if( alignment == Alignment::Center )
                {
                    currentPosition.x = position.x - lineWidths[line] * 0.5;
                }

                currentPosition.y += 115.0 * scale;
            }
            else if( c == ' ' )
            {
                currentPosition.x += 70.0 * scale;
            }
            else
            {
                int codepoint = c;
                Glyph glyph   = m_characterData[font]['?'];
                if( codepoint < m_characterData[font].size() )
                {
                    glyph = m_characterData[font][codepoint];
                }

                glm::vec2 basisA   = glm::vec2( 1.0, 0.0 ) * static_cast<float>( glyph.width ) * scale;
                glm::vec2 basisB   = glm::vec2( 0.0, 1.0 ) * static_cast<float>( glyph.height ) * scale;
                glm::u8vec4 color  = glm::u8vec4( textColor * 255.0f, 255 );
                glm::vec2 uvTop    = glm::vec2( glyph.x, glyph.y ) / static_cast<float>( AtlasWidth ) * 65535.0f;
                glm::vec2 uvBottom = glm::vec2( glyph.x + glyph.width, glyph.y + glyph.height ) / static_cast<float>( AtlasWidth ) * 65535.0f;
                // uvTop              = glm::vec2( 0.0, 0.0 );
                // uvBottom           = glm::vec2( 1.0, 1.0 ) * 65535.0f;
                glm::vec2 glyphPosition =
                    currentPosition + glm::vec2( glyph.width, glyph.height ) * 0.5f * scale + glm::vec2( glyph.xOffset, glyph.yOffset ) * scale;

                layerManager.add( { glyphPosition, basisA, basisB, uvTop, uvBottom, color, mc::HasSdfMaskTex, m_unitSquareMesh.start, m_unitSquareMesh.length,
                                    0, static_cast<uint16_t>( m_fontTextures.at( font ).resourceIndex() ) },
                                  ResourceHandle::invalidResource(), m_fontTextures.at( font ) );

                layerManager.data()[layerManager.length() - 1].outlineColor = glm::u8vec4( outlineColor * 255.0f, 255 );

                // outline needs to be between 0.0-0.5 but we make it a bit less because it looks bad at max thickness
                layerManager.data()[layerManager.length() - 1].outlineWidth = std::clamp<float>( outline / 2.0, 0.0, 0.45 );

                layerManager.data()[layerManager.length() - 1].fontSize = scale;

                currentPosition.x += glyph.xAdvance * scale;
            }
        }
    }

} // namespace mc