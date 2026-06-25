#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIClipSpace.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/RHI/Resources/RHITexture.h>
#include <XEngine/RHI/Resources/RHITextureView.h>

#include <cstddef>
#include <functional>
#include <memory>

namespace XEngine
{
    class RHICommandList;
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

        virtual std::shared_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) = 0;
        virtual std::shared_ptr<RHIBuffer> CreateBuffer(
            const RHIBufferDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) = 0;

        // TODO: Stage 8/10:
        // Split RHIDevice resource creation into RHIResourceFactory and texture uploads into RHIUploadManager.
        virtual std::shared_ptr<RHITexture> CreateTexture(
            const RHITextureDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) = 0;

        virtual std::shared_ptr<RHITextureView> CreateTextureView(
            const RHITextureViewDesc& desc) = 0;

        virtual std::shared_ptr<RHISampler> CreateSampler(
            const RHISamplerDesc& desc) = 0;

        // Update an existing bind group's sampled-texture binding in-place.
        // Used by RenderFrameResources to swap shadow texture / sampler when
        // ShadowResourceCache rebuilds the shadow array.
        virtual void UpdateBindGroupSampledTexture(
            RHIBindGroup* bindGroup,
            u32 binding,
            RHITextureView* view,
            RHISampler* sampler) = 0;  // TODO: is really needed to do it here

        virtual std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayout(
            const RHIBindGroupLayoutDesc& desc) = 0;

        virtual std::shared_ptr<RHIBindGroup> CreateBindGroup(
            const RHIBindGroupDesc& desc) = 0;

        virtual std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(
            const RHIGraphicsPipelineDesc& desc) = 0;

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
    };
}
