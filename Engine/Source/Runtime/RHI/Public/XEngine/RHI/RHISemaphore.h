// RHISemaphore — GPU-GPU synchronization primitive.
//
// M6 surface: empty (no CPU-side methods). Semaphores are signaled on
// the GPU side via RHIQueue::Submit with signalSemaphores array.
//
// Fence vs Semaphore:
//   - Fence:     CPU waits for GPU (signal on GPU, wait on CPU)
//   - Semaphore: GPU waits for GPU (signal on GPU A, wait on GPU B)
//
// M6 keeps the class empty so Submit's signature is consistent across
// backends. M7+ adds GetValue() for timeline semaphores.

#pragma once

#include <XEngine/RHI/RHIObject.h>

namespace XEngine
{
    class RHIDevice;  // forward

    class RHISemaphore : public RHIObject
    {
    public:
        virtual ~RHISemaphore() override = default;

        // Non-copyable / non-movable: semaphore wraps native handle
        // (VkSemaphore, ID3D12Fence-shared, MTLEvent).
        RHISemaphore(const RHISemaphore&) = delete;
        RHISemaphore& operator=(const RHISemaphore&) = delete;
        RHISemaphore(RHISemaphore&&) = delete;
        RHISemaphore& operator=(RHISemaphore&&) = delete;

    protected:
        explicit RHISemaphore(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHISemaphore(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };
}
