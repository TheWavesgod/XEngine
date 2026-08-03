// RHIEnums — central registry of RHI enumerations.
//
// Filled in incrementally:
//   M2: RHIAdapterPreference, RHIAdapterType
//   M3: RHIQueueType
//   M4: RHIFormat, RHIBufferUsage
//   M5: RHITextureUsage
//   M6: RHIPipelineStage
//   M7: RHIShaderStage
//   M10: RHIPresentMode
//
// Convention: all enums are `enum class : <small-int>` so the underlying
// type is fixed and ABI stable. Default values are 0 (None / first entry).

#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class RHIAdapterPreference : u8
    {
        Automatic,         // highest composite score (performance + power)
        HighPerformance,   // discrete / fastest GPU preferred
        LowPower,          // integrated / mobile GPU preferred
        Explicit,          // user picks by ID — M11 API, M2 only reserves the value
    };

    enum class RHIAdapterType : u8
    {
        Discrete,
        Integrated,
        CPU,
        Unknown,
    };

    // M3: queue classification. Mirrors Vulkan's graphics/compute/transfer
    // triplet and D3D12's direct/compute/copy triplet. A graphics queue may
    // also do compute and transfer; RHIQueueType reports the primary role.
    enum class RHIQueueType : u8
    {
        Graphics,  // graphics + compute + transfer
        Compute,   // compute + transfer
        Transfer,  // transfer-only
    };
}
