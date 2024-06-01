#pragma once

#include "app.h"

typedef union SDL_Event;

namespace wgpu
{
    class RenderPassEncoder;
}

namespace mc
{
    constexpr float HandleHalfSize     = 7.0;
    constexpr float HandleCornerRadius = 3.0;
    constexpr float RotateHandleHeight = 40.0;

    void initUI( AppContext* app );
    void drawUI( AppContext* app, const wgpu::RenderPassEncoder& renderPass );
    void processEventUI( const SDL_Event* event );
    bool captureMouseUI();
    void shutdownUI();
} // namespace mc