#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIClipSpace.h>

#include <cstddef>
#include <functional>
#include <memory>

namespace XEngine
{
    class RHICommandList;
    class RHIResourceFactory;
    struct VulkanNativeContext;
    using RHINativeCommandBuffer = void*;

    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        virtual RHIBackend GetBackend() const = 0;
        virtual RHIClipSpaceConvention GetClipSpaceConvention() const = 0;

        virtual bool IsValid() const = 0;

        virtual RHICommandList* BeginFrame() = 0;
        virtual void ClearSwapchain(const RHIColor& color) = 0;
        virtual void EndFrame() = 0;

        virtual void RequestResize(u32 width, u32 height) = 0;

        // Update an existing bind group's sampled-texture binding in-place.
        // Used by RenderFrameResources to swap shadow texture / sampler when
        // ShadowResourceCache rebuilds the shadow array.
        virtual void UpdateBindGroupSampledTexture(
            RHIBindGroup* bindGroup,
            u32 binding,
            RHITextureView* view,
            RHISampler* sampler) = 0;  // TODO: is really needed to do it here

        virtual RHIFormat GetSwapchainFormat() const = 0;

        virtual bool GetVulkanNativeContext(VulkanNativeContext& outContext) const
        {
            (void)outContext;
            return false;
        }

        virtual void RenderVulkanOverlay(const std::function<void(RHINativeCommandBuffer)>& callback)
        {
            (void)callback;
        }

        virtual void WaitIdle() = 0;

        virtual RHIResourceFactory& GetResourceFactory() = 0;
        virtual const RHIResourceFactory& GetResourceFactory() const = 0;
    };
}
