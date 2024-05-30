#pragma once

#include "app.h"
#include "imgui_config.h"

#include <imgui.h>
#include <webgpu/webgpu_cpp.h>

namespace mc
{
    constexpr float HandleHalfSize     = 7.0;
    constexpr float HandleCornerRadius = 3.0;

    void initUI( AppContext* app );
    void drawUI( AppContext* app, const wgpu::RenderPassEncoder& renderPass );
    void shutdownUI();
} // namespace mc