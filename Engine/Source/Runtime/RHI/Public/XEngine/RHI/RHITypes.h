#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class RHIBackend
    {
        None,
        Vulkan,
        D3D12,
        Metal
    };

    enum class RHIQueueType
    {
        Graphics,
        Compute,
        Transfer
    };

    enum class RHIFormat
    {
        Undefined,
        BGRA8Unorm,
        RGBA8Unorm,
        RGBA16Float,
        D32Float,
        R32G32Float,
        R32G32B32Float,
        R32G32B32A32Float
    };

    enum class RHIIndexFormat
    {
        UInt16,
        UInt32
    };

    struct RHIColor
    {
        f32 R = 0.0f;
        f32 G = 0.0f;
        f32 B = 0.0f;
        f32 A = 1.0f;
    };

    struct RHIPhysicalDeviceInfo
    {
        const char* Name = "";
        u32 VendorId = 0;
        u32 DeviceId = 0;
    };
}
