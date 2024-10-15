#pragma once

#include "font_manager.h"

#include <glm/glm.hpp>
#include <string>

typedef union SDL_Event;

namespace wgpu
{
    class RenderPassEncoder;
}

namespace mc
{
    struct AppContext;

    enum class Mode;

    enum class MouseLocationUI
    {
        None,
        Window,
        RotateHandle,
        ScaleHandleTL,
        ScaleHandleBR,
        ScaleHandleTR,
        ScaleHandleBL
    };

    constexpr float HandleHalfSize     = 7.0;
    constexpr float HandleCornerRadius = 3.0;
    constexpr float RotateHandleHeight = 40.0;

    void initUI( const AppContext* app );
    void setStylesUI( const AppContext* app );
    void setColorsUI();
    void drawUI( const AppContext* app, const wgpu::RenderPassEncoder& renderPass );
    void processEventUI( const AppContext* app, const SDL_Event* event );
    void shutdownUI();
    void changeModeUI( Mode newMode );

    MouseLocationUI getMouseLocationUI();

    glm::vec3 getPaintColor();
    float getPaintRadius();

    std::string getInputTextString();
    glm::vec3 getInputTextColor();
    glm::vec3 getInputTextOutlineColor();
    float getInputTextOutline();
    FontManager::Alignment getInputTextAlignment();
    FontManager::Font getInputTextFont();

    bool getSaveWithTransparency();
} // namespace mc