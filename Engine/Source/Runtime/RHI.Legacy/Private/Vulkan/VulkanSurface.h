#pragma once

#include <XEngine/Platform/NativeWindowHandle.h>

#include <volk.h>

#include <vector>

namespace XEngine
{
    class VulkanSurface
    {
    public:
        VulkanSurface() = default;
        ~VulkanSurface();

        static std::vector<const char*> GetRequiredInstanceExtensions();

        bool Create(VkInstance instance, NativeWindowHandle nativeWindow);
        void Destroy();

        VkSurfaceKHR GetHandle() const;

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    };
}
