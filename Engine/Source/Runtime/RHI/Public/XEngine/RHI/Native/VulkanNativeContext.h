#pragma once

#include <XEngine/Core/Types.h>

#include <volk.h>

namespace XEngine
{
    // Backend-specific bridge for editor integration. It exposes native Vulkan
    // handles without making Runtime Renderer depend on editor UI libraries.
    struct VulkanNativeContext
    {
        VkInstance Instance = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;
        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        u32 GraphicsQueueFamilyIndex = 0;
        u32 MinImageCount = 0;
        u32 ImageCount = 0;
        VkFormat ColorFormat = VK_FORMAT_UNDEFINED;
        VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
    };
}
