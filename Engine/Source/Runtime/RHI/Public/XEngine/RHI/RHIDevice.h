// RHIDevice — M3 real interface.
//
// M3 surface (lifecycle + queue + capabilities):
//   * WaitIdle()               — block until all queue submissions finish
//   * GetBackend()             — backend tag
//   * GetCapabilities()        — const ref to internal caps struct
//   * GetMaxFramesInFlight()   — frames in flight (typical 2 or 3)
//   * GetQueue(RHIQueueType)   — queue by primary type
//
// M4+ extends with resource creation (CreateBuffer, CreateTexture, ...).
// RHIDeviceDesc is in RHIDescriptors.h (Option D1: descs centralized).

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIDescriptors.h>

namespace XEngine
{
    class RHIInstance;
    class RHIQueue;  // forward — RHIQueue.h is not included here to avoid circular include

    // RHICapabilities — device capability set.
    //
    // M3 baseline: basic limits + 7 feature flags from audit §3.4 + alignment.
    // Backend implementations populate this in their Initialize() / before
    // returning from RHIInstance::CreateDevice.
    //
    // Field defaults:
    //   - limits: 0 (meaning "unset / unknown"); backend must set true value
    //   - alignment: 1 (no alignment requirement); backend sets actual value
    //   - features: false (capability not exposed); backend sets true/false
    struct RHICapabilities
    {
        // Basic limits
        u32 MaxTextureSize2D             = 0;
        u32 MaxBufferSize                = 0;
        u32 MaxSamplerAnisotropy         = 0;
        u32 MaxSampleCount               = 0;
        u32 MaxViewports                 = 1;     // 1 is the minimum
        u32 MaxColorAttachments          = 0;

        // Frame sync (audit §3.2 — single source of truth)
        u32 MaxFramesInFlight            = 1;

        // Compute
        u32 MaxComputeWorkGroupInvocations = 0;
        u32 MaxComputeSharedMemorySize     = 0;

        // Alignment requirements (in bytes; 1 = no alignment requirement)
        u64 MinUniformBufferOffsetAlignment = 1;
        u64 MinStorageBufferOffsetAlignment = 1;
        u64 MinTexelBufferOffsetAlignment    = 1;

        // Feature flags (audit §3.4 — must include all 7 baseline items)
        bool SupportsDepthClip            = true;  // exception: D3D12 always supports
        bool SupportsDepthBiasClamp       = false;
        bool SupportsWideLines            = false;
        bool SupportsLargePoints          = false;
        bool SupportsTimelineSemaphore   = false;  // Vulkan 1.2+
        bool SupportsPushDescriptor      = false;  // Vulkan ext
        bool SupportsBindless             = false;
        bool SupportsBufferDeviceAddress = false;  // Vulkan 1.2 / D3D12
        bool SupportsRayTracing          = false;  // future (M13+)
        bool SupportsGeometryShader      = false;  // future
        bool SupportsTessellationShader  = false;  // future
    };

    // RHIDevice — M3 real interface.
    class RHIDevice : public RHIObject
    {
    public:
        // Lifecycle
        virtual void WaitIdle() = 0;

        // Device info
        virtual RHIBackend GetBackend() const noexcept = 0;
        virtual const RHICapabilities& GetCapabilities() const noexcept = 0;
        virtual u32 GetMaxFramesInFlight() const noexcept = 0;

        // Queues
        virtual RHIQueue* GetQueue(RHIQueueType type) const = 0;

        virtual ~RHIDevice() override = default;

        // Non-copyable: device wraps native handles (VkDevice, ID3D12Device,
        // id<MTLDevice>) that are not safely copyable.
        RHIDevice(const RHIDevice&) = delete;
        RHIDevice& operator=(const RHIDevice&) = delete;

    protected:
        explicit RHIDevice(RHIInstance& owner) noexcept
            : RHIObject()
        {
        }

        RHIDevice(RHIInstance& owner, RHIBackend backend) noexcept
            : RHIObject(backend)
        {
        }
    };
}
