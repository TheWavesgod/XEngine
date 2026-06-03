#pragma once

#include <XEngine/Platform/NativeWindowHandle.h>
#include <XEngine/RHI/RHIDevice.h>

#include "VulkanAllocator.h"
#include "VulkanInstance.h"
#include "VulkanQueue.h"
#include "VulkanSurface.h"

#include <volk.h>

namespace XEngine
{
    struct VulkanDeviceCreateInfo
    {
        NativeWindowHandle NativeWindow;
        bool EnableValidation = true;
        bool EnableVSync = true;
    };

    class VulkanDevice final : public RHIDevice
    {
    public:
        VulkanDevice();
        ~VulkanDevice() override;

        bool Initialize(const VulkanDeviceCreateInfo& createInfo);
        void Shutdown();

        RHIBackend GetBackend() const override;
        bool IsValid() const override;
        void WaitIdle() override;

    private:
        bool PickPhysicalDevice();
        bool CreateLogicalDevice();

        VulkanInstance m_Instance;
        VulkanSurface m_Surface;
        VulkanAllocator m_Allocator;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VulkanQueue m_GraphicsQueue;
        VulkanQueue m_PresentQueue;

        u32 m_GraphicsFamilyIndex = 0;
        u32 m_PresentFamilyIndex = 0;
        bool m_Initialized = false;
    };
}
