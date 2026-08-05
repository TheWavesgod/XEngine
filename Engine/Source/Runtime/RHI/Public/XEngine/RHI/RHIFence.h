// RHIFence — CPU-GPU synchronization primitive.
//
// M6 surface (lifecycle + query):
//   * IsSignaled()    — non-blocking signal check
//   * Wait(timeout)   — CPU blocks until signal (or timeout)
//   * Reset() deferred — M6 uses single-use fences, M11+ adds reuse
//
// Fence vs Semaphore (audit 3.7 related):
//   - Fence:    CPU waits for GPU (signal on GPU, wait on CPU)
//   - Semaphore: GPU waits for GPU (signal on GPU A, wait on GPU B)

#pragma once

#include <XEngine/RHI/RHIObject.h>

namespace XEngine
{
    class RHIDevice;  // forward — ctor takes RHIDevice&

    class RHIFence : public RHIObject
    {
    public:
        virtual ~RHIFence() override = default;

        // Non-blocking signal check.
        virtual bool IsSignaled() const noexcept = 0;

        // Blocks until GPU signals this fence (or timeout). Returns true if
        // signaled, false on timeout.
        virtual bool Wait(u64 timeoutNanoseconds = UINT64_MAX) noexcept = 0;

        // Non-copyable / non-movable: fence wraps native handle (VkFence,
        // ID3D12Fence, MTLFence).
        RHIFence(const RHIFence&) = delete;
        RHIFence& operator=(const RHIFence&) = delete;
        RHIFence(RHIFence&&) = delete;
        RHIFence& operator=(RHIFence&&) = delete;

    protected:
        explicit RHIFence(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHIFence(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };
}
