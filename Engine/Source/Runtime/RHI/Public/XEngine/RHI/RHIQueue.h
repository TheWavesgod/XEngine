// RHIQueue — command queue.
//
// M3 phase: only identification (GetType). Submit / Fence / Semaphore
// operations land in M6; signaling / waiting on M6; present on M10.
//
// Lives on device: created by concrete backend inside RHIInstance::CreateDevice,
// retrieved by user via RHIDevice::GetQueue(RHIQueueType).

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIEnums.h>

namespace XEngine
{
    // Forward declaration — RHIQueue's constructor takes RHIDevice&.
    class RHIDevice;

    // RHIQueue — command queue.
    class RHIQueue : public RHIObject
    {
    public:
        virtual ~RHIQueue() override = default;

        // Returns the queue's primary classification. The actual capabilities
        // (graphics queue may also do compute/transfer) are backend-specific
        // and not surfaced at this layer in M3.
        virtual RHIQueueType GetType() const noexcept = 0;

        // Non-copyable: backend queues wrap native handles that are not
        // safely copyable (Vulkan VkQueue, D3D12 ID3D12CommandQueue, ...).
        RHIQueue(const RHIQueue&) = delete;
        RHIQueue& operator=(const RHIQueue&) = delete;

    protected:
        explicit RHIQueue(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHIQueue(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };
}
