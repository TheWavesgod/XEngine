// RHIDevice — M3+M4+M5+M6 interface.
//
// M3 surface (lifecycle + queue + capabilities):
//   * WaitIdle()               — block until all queue submissions finish
//   * GetBackend()             — backend tag
//   * GetCapabilities()        — const ref to internal caps struct
//   * GetMaxFramesInFlight()   — frames in flight (typical 2 or 3)
//   * GetQueue(RHIQueueType)   — queue by primary type
//
// M4 surface (resource creation):
//   * CreateBuffer(RHIBufferDesc) — NVI wrapper → CreateBufferImpl
//
// M5 surface (resource creation):
//   * CreateTexture(RHITextureDesc)              — NVI wrapper → CreateTextureImpl
//   * CreateTextureView(RHITextureViewDesc)      — NVI wrapper → CreateTextureViewImpl
//   * CreateSampler(RHISamplerDesc)              — NVI wrapper → CreateSamplerImpl
//
// M6 surface (sync primitives):
//   * CreateFence(RHIFenceDesc)                  — NVI wrapper → CreateFenceImpl
//   * CreateSemaphore(RHISemaphoreDesc)          — NVI wrapper → CreateSemaphoreImpl
//   * CreateCommandList(RHICommandListDesc)      — NVI wrapper → CreateCommandListImpl
//
// M7+ extends with CreateShader, BindGroup, Pipeline, etc.
//
// NVI wrapper bodies live in RHIDevice.cpp.

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIAdapter.h>
#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHITexture.h>
#include <XEngine/RHI/RHISampler.h>

namespace XEngine
{
    class RHIInstance;
    class RHIQueue;        // forward
    class RHICommandList;  // forward
    class RHIFence;        // forward
    class RHISemaphore;    // forward

    // RHICapabilities — device capability set.
    struct RHICapabilities
    {
        u32 MaxTextureSize2D             = 0;
        u32 MaxBufferSize                = 0;
        u32 MaxSamplerAnisotropy         = 0;
        u32 MaxSampleCount               = 0;
        u32 MaxViewports                 = 1;
        u32 MaxColorAttachments          = 0;

        u32 MaxFramesInFlight            = 1;

        u32 MaxComputeWorkGroupInvocations = 0;
        u32 MaxComputeSharedMemorySize     = 0;

        u64 MinUniformBufferOffsetAlignment = 1;
        u64 MinStorageBufferOffsetAlignment = 1;
        u64 MinTexelBufferOffsetAlignment    = 1;

        bool SupportsDepthClip            = true;
        bool SupportsDepthBiasClamp       = false;
        bool SupportsWideLines            = false;
        bool SupportsLargePoints          = false;
        bool SupportsTimelineSemaphore   = false;
        bool SupportsPushDescriptor      = false;
        bool SupportsBindless             = false;
        bool SupportsBufferDeviceAddress = false;
        bool SupportsRayTracing          = false;
        bool SupportsGeometryShader      = false;
        bool SupportsTessellationShader  = false;
    };

    // RHIDevice — full M3-M6 interface.
    class RHIDevice : public RHIObject
    {
    public:
        // ---------- Lifecycle ----------
        virtual void WaitIdle() = 0;

        // ---------- Device info ----------
        virtual RHIBackend GetBackend() const noexcept = 0;
        virtual const RHICapabilities& GetCapabilities() const noexcept = 0;
        virtual u32 GetMaxFramesInFlight() const noexcept = 0;

        // ---------- Queues ----------
        virtual RHIQueue* GetQueue(RHIQueueType type) const = 0;

        // ---------- Resource creation (NVI) ----------
        // M4: buffer
        RHIBuffer* CreateBuffer(const RHIBufferDesc& desc);

        // M5: texture
        RHITexture* CreateTexture(const RHITextureDesc& desc);
        RHITextureView* CreateTextureView(const RHITextureViewDesc& desc);
        RHISampler* CreateSampler(const RHISamplerDesc& desc);

        // M6: sync primitives
        RHIFence* CreateFence(const RHIFenceDesc& desc);
        RHISemaphore* CreateSemaphore(const RHISemaphoreDesc& desc = RHISemaphoreDesc{});
        RHICommandList* CreateCommandList(const RHICommandListDesc& desc);

        // ---------- End ----------
        virtual ~RHIDevice() override = default;

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

        // M4: virtual hooks for resource creation.
        virtual RHIBuffer* CreateBufferImpl(const RHIBufferDesc& desc) = 0;

        // M5: virtual hooks.
        virtual RHITexture* CreateTextureImpl(const RHITextureDesc& desc) = 0;
        virtual RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc& desc) = 0;
        virtual RHISampler* CreateSamplerImpl(const RHISamplerDesc& desc) = 0;

        // M6: virtual hooks for sync primitives.
        virtual RHIFence* CreateFenceImpl(const RHIFenceDesc& desc) = 0;
        virtual RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc& desc) = 0;
        virtual RHICommandList* CreateCommandListImpl(const RHICommandListDesc& desc) = 0;
    };
}
