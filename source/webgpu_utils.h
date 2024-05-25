#pragma once

#include "app.h"

#include <webgpu/webgpu_cpp.h>

namespace mc
{
    void initMainPipeline( mc::AppContext* app );
    void initSwapChain( mc::AppContext* app );
    wgpu::Device requestDevice( const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor* descriptor );
    wgpu::Adapter requestAdapter( const wgpu::Instance& instance, const wgpu::RequestAdapterOptions* options );
} // namespace mc