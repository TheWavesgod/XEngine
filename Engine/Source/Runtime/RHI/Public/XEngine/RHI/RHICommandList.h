// RHICommandList — command recording buffer.
//
// M6 surface (lifecycle + barrier):
//   * Begin()  / End()             — recording lifecycle
//   * TransitionTexture(...)        — explicit texture layout transition (audit 3.7)
//
// M8+ adds: Draw / DrawIndexed / Dispatch
// M9+ adds: SetPipeline / SetBindGroup / render-pass commands
//
// M6 uses single-use command lists (Begin / End / Submit). M11+ may add
// reuse via Reset() if performance demands.

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIEnums.h>

namespace XEngine
{
    class RHIDevice;     // forward
    class RHITexture;    // forward

    class RHICommandList : public RHIObject
    {
    public:
        virtual ~RHICommandList() override = default;

        // Recording lifecycle. Backend must be in valid state to call.
        virtual void Begin() = 0;
        virtual void End() = 0;

        // Audit 3.7: explicit texture layout transition. Backends translate
        // (texture, oldLayout, newLayout, srcAccess, dstAccess) into the
        // appropriate barrier command (Vulkan: VkImageMemoryBarrier2,
        // D3D12: aliasing barrier, Metal: MTLBlitCommandEncoder).
        virtual void TransitionTexture(
            RHITexture* texture,
            RHIImageLayout oldLayout,
            RHIImageLayout newLayout,
            RHIAccessFlags srcAccess,
            RHIAccessFlags dstAccess) = 0;

        // Non-copyable / non-movable: command list wraps native handle
        // (VkCommandBuffer, ID3D12GraphicsCommandList, MTLCommandBuffer).
        RHICommandList(const RHICommandList&) = delete;
        RHICommandList& operator=(const RHICommandList&) = delete;
        RHICommandList(RHICommandList&&) = delete;
        RHICommandList& operator=(RHICommandList&&) = delete;

    protected:
        explicit RHICommandList(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHICommandList(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };
}
