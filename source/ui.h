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

    enum Alignment;

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

    enum class Mode
    {
        Cursor,
        Pan,
        Paint,
        Text,
        Other
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

    MouseLocationUI getMouseLocationUI();
    Mode getAppMode();

    glm::vec3 getPaintColor();
    float getPaintRadius();

    std::string getInputTextString();
    glm::vec3 getInputTextColor();
    glm::vec3 getInputTextOutlineColor();
    float getInputTextOutline();
    FontManager::Alignment getInputTextAlignment();
    FontManager::Font getInputTextFont();
} // namespace mc