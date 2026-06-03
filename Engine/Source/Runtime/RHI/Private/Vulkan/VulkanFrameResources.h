#pragma once

#include <XEngine/Core/Types.h>

#include <volk.h>

#include <vector>

namespace XEngine
{
    static constexpr u32 MaxFramesInFlight = 1;

    class VulkanFrameResources
    {
    public:
        VulkanFrameResources() = default;
        ~VulkanFrameResources();

        bool Create(VkDevice device, u32 graphicsQueueFamilyIndex, u32 swapchainImageCount);
        void Destroy();

        VkCommandPool GetCommandPool() const;
        VkCommandBuffer GetCommandBuffer() const;
        VkSemaphore GetImageAvailableSemaphore() const;
        VkSemaphore GetRenderFinishedSemaphore(u32 imageIndex) const;
        VkFence GetInFlightFence() const;
        u32 GetRenderFinishedSemaphoreCount() const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

        VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        VkFence m_InFlightFence = VK_NULL_HANDLE;
    };
}
