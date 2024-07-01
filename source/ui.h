#pragma once

#include <glm/glm.hpp>

typedef union SDL_Event;

namespace wgpu
{
    class RenderPassEncoder;
}

namespace mc
{
    struct AppContext;

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
    void setStylesUI( float dpiFactor );
    void setColorsUI();
    void drawUI( const AppContext* app, const wgpu::RenderPassEncoder& renderPass );
    void processEventUI( const AppContext* app, const SDL_Event* event );
    void shutdownUI();

    MouseLocationUI getMouseLocationUI();
    Mode getAppMode();
    glm::vec3 getPaintColor();
    float getPaintRadius();
} // namespace mc