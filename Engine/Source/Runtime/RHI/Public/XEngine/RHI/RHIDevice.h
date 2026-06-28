#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIClipSpace.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace XEngine
{
    class RHICommandList;
    class RHIBindGroup;
    class RHIResourceFactory;
    class RHISampler;
    class RHITextureView;
    class RHIUploadManager;
    struct VulkanNativeContext;
    struct VulkanNativeTextureBinding;
    using RHINativeCommandBuffer = std::uintptr_t;

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

        virtual RHIFormat GetSwapchainFormat() const = 0;

        virtual bool GetVulkanNativeContext(VulkanNativeContext& outContext) const
        {
            (void)outContext;
            return false;
        }

        virtual bool GetVulkanNativeTextureBinding(
            const RHISampler& sampler,
            const RHITextureView& textureView,
            VulkanNativeTextureBinding& outBinding) const
        {
            (void)sampler;
            (void)textureView;
            (void)outBinding;
            return false;
        }

        virtual void RenderVulkanOverlay(const std::function<void(RHINativeCommandBuffer)>& callback)
        {
            (void)callback;
        }

        virtual void WaitIdle() = 0;

        virtual RHIResourceFactory& GetResourceFactory() = 0;
        virtual const RHIResourceFactory& GetResourceFactory() const = 0;

        virtual RHIUploadManager& GetUploadManager() = 0;             
        virtual const RHIUploadManager& GetUploadManager() const = 0; 

        virtual const RHICapabilities& GetCapabilities() const = 0;
    };
}
