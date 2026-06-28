#pragma once

#include <XEngine/Core/Types.h>

#include <cstdint>

namespace XEngine
{
    // Narrow editor/backend bridge. Core RHI public headers remain free of
    // Vulkan headers and Vulkan types.
    struct VulkanNativeContext
    {
        std::uintptr_t Instance = 0;
        std::uintptr_t PhysicalDevice = 0;
        std::uintptr_t Device = 0;
        std::uintptr_t GraphicsQueue = 0;
        u32 GraphicsQueueFamilyIndex = 0;
        u32 MinImageCount = 0;
        u32 ImageCount = 0;
        u32 ColorFormat = 0;
        u32 DepthFormat = 0;
    };

    struct VulkanNativeTextureBinding
    {
        std::uintptr_t Sampler = 0;
        std::uintptr_t ImageView = 0;
    };
}
