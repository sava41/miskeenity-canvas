#pragma once

#include "font_manager.h"

#include <glm/glm.hpp>
#include <string>

union SDL_Event;

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
        MoveHandle,
        RotateHandle,
        ScaleHandleTL,
        ScaleHandleBR,
        ScaleHandleTR,
        ScaleHandleBL
    };

    struct TransformBox
    {
        glm::vec2 cornerHandleTL;
        glm::vec2 cornerHandleBR;
        glm::vec2 cornerHandleTR;
        glm::vec2 cornerHandleBL;
        glm::vec2 rotationHandle;
        glm::vec2 rotationHandleBase;
        bool insideBox;
    };

    const float HandleHalfSize     = 7.0;
    const float HandleCornerRadius = 3.0;
    const float RotateHandleHeight = 40.0;

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