// VulkanQueue — implementation.

#include "VulkanQueue.h"
#include "VulkanDevice.h"

#include <XEngine/Logging/Log.h>

#include <volk.h>

namespace XEngine
{
    VulkanQueue::VulkanQueue(VulkanDevice& device, VkQueue queue, RHIQueueType type)
        : RHIQueue(device, device.GetBackend())
        , m_Type(type)
        , m_Queue(queue)
    {
    }

    VulkanQueue::~VulkanQueue() = default;

    void VulkanQueue::Submit(
        RHICommandList* commandList,
        RHIFence* signalFence,
        std::span<RHISemaphore*> waitSemaphores,
        std::span<RHISemaphore*> signalSemaphores)
    {
        (void)commandList;
        (void)signalFence;
        (void)waitSemaphores;
        (void)signalSemaphores;
        // Phase 1 (M0-M3): M6 Submit is not yet implemented. Log and no-op so
        // the test harness can verify the dispatch without GPU work.
        XENGINE_LOG_WARN("VulkanQueue::Submit: not yet implemented (M6).");
    }
}
