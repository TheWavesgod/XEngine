#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <cstddef>
#include <memory>

namespace XEngine
{
    class RHICommandList;

    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        virtual RHIBackend GetBackend() const = 0;

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

        // TODO Stage 8/10:
        // Split RHIDevice resource creation into RHIResourceFactory and texture uploads into RHIUploadManager.
        virtual std::shared_ptr<RHITexture> CreateTexture(
            const RHITextureDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) = 0;

        virtual std::shared_ptr<RHISampler> CreateSampler(
            const RHISamplerDesc& desc) = 0;

        virtual std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(
            const RHIGraphicsPipelineDesc& desc) = 0;

        virtual RHIFormat GetSwapchainFormat() const = 0;

        virtual void WaitIdle() = 0;
    };
}
