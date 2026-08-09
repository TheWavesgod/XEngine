// VulkanQueue — concrete RHIQueue for the Vulkan backend.
//
// Wraps a VkQueue with its family index. Phase 1 implements only GetType
// (M3); Submit (M6) is a stub that logs the call.

#pragma once

#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIEnums.h>

#include <vulkan/vulkan.h>

#include <span>

namespace XEngine
{
    class VulkanDevice;

    class VulkanQueue : public RHIQueue
    {
    public:
        // Used by XEngine::CheckedCast<T> to reject cross-backend casts.
        static constexpr RHIBackend ExpectedBackend = RHIBackend::Vulkan;

        VulkanQueue(VulkanDevice& device, VkQueue queue, RHIQueueType type);
        ~VulkanQueue() override;

        RHIQueueType GetType() const noexcept override { return m_Type; }

        // M6: stub for now. Will translate to vkQueueSubmit.
        void Submit(
            RHICommandList* commandList,
            RHIFence* signalFence = nullptr,
            std::span<RHISemaphore*> waitSemaphores = {},
            std::span<RHISemaphore*> signalSemaphores = {}) override;

        // Vulkan-specific accessors
        VkQueue GetVkQueue() const noexcept { return m_Queue; }

    private:
        RHIQueueType m_Type;
        VkQueue m_Queue = VK_NULL_HANDLE;
    };
}
