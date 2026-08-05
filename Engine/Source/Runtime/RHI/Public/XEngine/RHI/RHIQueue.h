// RHIQueue — command queue.
//
// M3 phase: only identification (GetType).
// M6 phase: + Submit (full CPU-GPU + GPU-GPU sync support).
//
// Submit cross-backend behavior:
//   - Vulkan:  VkQueueSubmit with VkSubmitInfo (fence + semaphore arrays)
//   - D3D12:   ExecuteCommandLists + Signal(fence, value); semaphores map
//              to additional ID3D12Fence handles (value-based)
//   - Metal:   MTLCommandBuffer commit; waitSem/signalSem translate to
//              MTLEvent wait/signal encoded into command buffer

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIEnums.h>

#include <span>

namespace XEngine
{
    class RHIFence;         // forward
    class RHICommandList;   // forward
    class RHISemaphore;     // forward
    class RHIDevice;        // forward

    class RHIQueue : public RHIObject
    {
    public:
        virtual ~RHIQueue() override = default;

        // Returns the queue's primary classification.
        virtual RHIQueueType GetType() const noexcept = 0;

        // M6: Submit a command list with full sync support.
        //   * commandList:        the records to play (must be in End'd state)
        //   * signalFence:        CPU-GPU sync (signaled when this queue completes)
        //   * waitSemaphores:     GPU-GPU sync (this queue waits for these)
        //   * signalSemaphores:   GPU-GPU sync (this queue signals these on completion)
        //
        // M11+ may add a multi-commandList overload (array of command lists).
        virtual void Submit(
            RHICommandList* commandList,
            RHIFence* signalFence = nullptr,
            std::span<RHISemaphore*> waitSemaphores = {},
            std::span<RHISemaphore*> signalSemaphores = {}) = 0;

        // Non-copyable: backend queues wrap native handles that are not
        // safely copyable (Vulkan VkQueue, D3D12 ID3D12CommandQueue,
        // MTLCommandQueue).
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
