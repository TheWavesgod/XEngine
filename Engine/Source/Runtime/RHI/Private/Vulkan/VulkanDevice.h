#pragma once

#include <XEngine/Platform/NativeWindowHandle.h>
#include <XEngine/RHI/RHIDevice.h>

#include "VulkanAllocator.h"
#include "VulkanFrameResources.h"
#include "VulkanInstance.h"
#include "VulkanQueue.h"
#include "VulkanSurface.h"
#include "VulkanSwapchain.h"

#include <volk.h>

namespace XEngine
{
    struct VulkanDeviceCreateInfo
    {
        NativeWindowHandle NativeWindow;
        u32 Width = 1280;
        u32 Height = 720;
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
        void BeginFrame() override;
        void ClearSwapchain(const RHIColor& color) override;
        void EndFrame() override;
        void RequestResize(u32 width, u32 height) override;
        void WaitIdle() override;

    private:
        bool PickPhysicalDevice();
        bool CreateLogicalDevice();
        void RecreateSwapchain(u32 width, u32 height);

        VulkanInstance m_Instance;
        VulkanSurface m_Surface;
        VulkanAllocator m_Allocator;
        VulkanSwapchain m_Swapchain;
        VulkanFrameResources m_FrameResources;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VulkanQueue m_GraphicsQueue;
        VulkanQueue m_PresentQueue;

        u32 m_GraphicsFamilyIndex = 0;
        u32 m_PresentFamilyIndex = 0;
        u32 m_CurrentImageIndex = 0;
        u32 m_PendingResizeWidth = 0;
        u32 m_PendingResizeHeight = 0;
        bool m_EnableVSync = true;
        bool m_FrameActive = false;
        bool m_ResizeRequested = false;
        bool m_Initialized = false;
    };
}
