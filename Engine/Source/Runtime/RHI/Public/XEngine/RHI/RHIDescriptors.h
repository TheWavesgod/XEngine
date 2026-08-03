// RHIDescriptors — central registry of resource creation parameter structs.
//
// Filled in incrementally:
//   M2: RHIInstanceDesc, RHIDeviceDesc
//   M3: RHIQueueDesc
//   M4: RHIBufferDesc
//   M5: RHITextureDesc, RHISamplerDesc
//   M6: RHICommandListDesc
//   M7: RHIShaderDesc, RHIBindGroupLayoutDesc, RHIBindGroupDesc
//   M8: RHIComputePipelineDesc
//   M9: RHIGraphicsPipelineDesc
//   M10: RHISwapchainDesc
//
// Convention: each desc is a POD struct with sensible defaults so empty
// braces work as a default. `DebugName` is added per-desc as needed.

#pragma once

#include <XEngine/Core/Types.h>

#include <string_view>

namespace XEngine
{
    struct RHIInstanceDesc
    {
        std::string_view ApplicationName    = "XEngineApp";
        u32              ApplicationVersion = 1;
        bool             EnableValidation   = false;
        bool             EnableDebugMarkers = true;
    };

    struct RHIDeviceDesc
    {
    };
}
