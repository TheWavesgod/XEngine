#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/RHI/Resources/RHITexture.h>
#include <XEngine/RHI/Resources/RHITextureView.h>

#include <memory>

namespace XEngine
{
    class RHIDevice;

    // NVI-style base. Public methods do common validation / normalization,
    // then call the protected virtual CreateXImpl that the backend implements.
    class RHIResourceFactory
    {
    public:
        virtual ~RHIResourceFactory() = default;

        std::shared_ptr<RHIBuffer> CreateBuffer(
            const RHIBufferDesc& desc,
            const void* initialData = nullptr,
            std::size_t initialDataSize = 0);

        std::shared_ptr<RHITexture> CreateTexture(
            const RHITextureDesc& desc,
            const void* initialData = nullptr,
            std::size_t initialDataSize = 0);

        std::shared_ptr<RHITextureView> CreateTextureView(
            const RHITextureViewDesc& desc);

        std::shared_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc);
        std::shared_ptr<RHIShader>  CreateShader(const RHIShaderDesc& desc);

        std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayout(
            const RHIBindGroupLayoutDesc& desc);

        std::shared_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc);

        std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(
            const RHIGraphicsPipelineDesc& desc);

        RHIDevice& GetDevice() const;

    protected:
        explicit RHIResourceFactory(RHIDevice& ownerDevice);

        virtual std::shared_ptr<RHIBuffer> CreateBufferImpl(
            const RHIBufferDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) = 0;

        virtual std::shared_ptr<RHITexture> CreateTextureImpl(
            const RHITextureDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) = 0;

        virtual std::shared_ptr<RHITextureView> CreateTextureViewImpl(
            const RHITextureViewDesc& desc) = 0;

        virtual std::shared_ptr<RHISampler> CreateSamplerImpl(
            const RHISamplerDesc& desc) = 0;

        virtual std::shared_ptr<RHIShader> CreateShaderImpl(
            const RHIShaderDesc& desc) = 0;

        virtual std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayoutImpl(
            const RHIBindGroupLayoutDesc& desc) = 0;

        virtual std::shared_ptr<RHIBindGroup> CreateBindGroupImpl(
            const RHIBindGroupDesc& desc) = 0;

        virtual std::shared_ptr<RHIPipeline> CreateGraphicsPipelineImpl(
            const RHIGraphicsPipelineDesc& desc) = 0;

    private:
        RHIDevice* m_Device = nullptr;
    }; 
}