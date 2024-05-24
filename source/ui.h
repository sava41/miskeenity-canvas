#pragma once

#include "app.h"

#include <webgpu/webgpu_cpp.h>

namespace mc
{
    void initUI( AppContext* app );
    void drawUI( AppContext* app, const wgpu::RenderPassEncoder& renderPass );
    void shutdownUI();
} // namespace mc