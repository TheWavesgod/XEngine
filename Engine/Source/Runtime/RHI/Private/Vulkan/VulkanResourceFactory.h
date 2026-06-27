#pragma once

#include <XEngine/RHI/RHIResourceFactory.h>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <cstddef>
#include <memory>

namespace XEngine
{
    class VulkanDevice;

    class VulkanResourceFactory final : public RHIResourceFactory
    {
    public:
        explicit VulkanResourceFactory(VulkanDevice& ownerDevice);
        ~VulkanResourceFactory() override = default;

    protected:
        std::shared_ptr<RHIBuffer> CreateBufferImpl(
            const RHIBufferDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) override;

        std::shared_ptr<RHITexture> CreateTextureImpl(
            const RHITextureDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) override;

        std::shared_ptr<RHITextureView> CreateTextureViewImpl(
            const RHITextureViewDesc& desc) override;

        std::shared_ptr<RHISampler> CreateSamplerImpl(
            const RHISamplerDesc& desc) override;

        std::shared_ptr<RHIShader> CreateShaderImpl(
            const RHIShaderDesc& desc) override;

        std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayoutImpl(
            const RHIBindGroupLayoutDesc& desc) override;

        std::shared_ptr<RHIBindGroup> CreateBindGroupImpl(
            const RHIBindGroupDesc& desc) override;

        std::shared_ptr<RHIPipeline> CreateGraphicsPipelineImpl(
            const RHIGraphicsPipelineDesc& desc) override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
}