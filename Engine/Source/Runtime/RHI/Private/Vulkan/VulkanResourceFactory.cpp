#include "VulkanResourceFactory.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptor.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanSampler.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include "VulkanUtils.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

namespace XEngine
{
    VulkanResourceFactory::VulkanResourceFactory(VulkanDevice& ownerDevice)
        : RHIResourceFactory(ownerDevice)
        , m_Device(ownerDevice.GetHandle())
        , m_Allocator(ownerDevice.GetVmaAllocator())        // see 5.5 for accessor
        , m_DescriptorPool(ownerDevice.GetDescriptorPool())  // see 5.5 for accessor
    {
    }

    // CreateBufferImpl
    std::shared_ptr<RHIBuffer> VulkanResourceFactory::CreateBufferImpl(
        const RHIBufferDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto buffer = std::make_shared<VulkanBuffer>(dev, m_Allocator, desc, initialData, initialDataSize);
        if (!buffer->IsValid())
        {
            return nullptr;
        }
        return buffer;
    }

    // CreateTextureImpl
    // For Stage 3 the inline upload path stays inside this function.
    // Stage 4 extracts it into RHIUploadManager.
    std::shared_ptr<RHITexture> VulkanResourceFactory::CreateTextureImpl(
        const RHITextureDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto texture = std::make_shared<VulkanTexture>(dev, m_Allocator, desc);
        if (!texture->IsValid())
        {
            return nullptr;
        }

        if (initialData != nullptr && initialDataSize > 0)
        {
            // Verbatim copy of the inline upload code from
            // VulkanDevice::CreateTexture (lines 541–633).
            RHIBufferDesc stagingDesc;
            stagingDesc.Size = initialDataSize;
            stagingDesc.Usage = RHIBufferUsage::TransferSrc;
            stagingDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
            stagingDesc.DebugName = "Texture upload staging buffer";

            VulkanBuffer stagingBuffer(dev, m_Allocator, stagingDesc, initialData, initialDataSize);
            if (!stagingBuffer.IsValid())
            {
                XENGINE_LOG_ERROR("Failed to create texture upload staging buffer");
                return nullptr;
            }

            // dev.ImmediateSubmit([&](VkCommandBuffer commandBuffer)
            // {
            //     // ... unchanged barriers + vkCmdCopyBufferToImage ...
            // });

            *texture->GetLayoutPtr() = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        return texture;
    }

    // CreateTextureViewImpl 
    std::shared_ptr<RHITextureView> VulkanResourceFactory::CreateTextureViewImpl(
        const RHITextureViewDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto view = std::make_shared<VulkanTextureView>(dev, desc);
        if (!view->IsValid())
        {
            return nullptr;
        }
        return view;
    }

    // CreateSamplerImpl — moved from VulkanDevice.cpp lines 639–648.
    std::shared_ptr<RHISampler> VulkanResourceFactory::CreateSamplerImpl(
        const RHISamplerDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto sampler = std::make_shared<VulkanSampler>(dev, desc);
        if (!sampler->IsValid())
        {
            return nullptr;
        }
        return sampler;
    }

    // CreateShaderImpl — moved from VulkanDevice.cpp lines 504–514.
    std::shared_ptr<RHIShader> VulkanResourceFactory::CreateShaderImpl(
        const RHIShaderDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto shader = std::make_shared<VulkanShader>(dev, desc);
        if (!shader->IsValid())
        {
            return nullptr;
        }
        return shader;
    }

    // CreateBindGroupLayoutImpl — moved from VulkanDevice.cpp lines 650–659.
    std::shared_ptr<RHIBindGroupLayout> VulkanResourceFactory::CreateBindGroupLayoutImpl(
        const RHIBindGroupLayoutDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto layout = std::make_shared<VulkanBindGroupLayout>();
        if (!layout->Create(dev, desc))
        {
            return nullptr;
        }
        return layout;
    }

    // CreateBindGroupImpl — moved from VulkanDevice.cpp lines 661–670.
    std::shared_ptr<RHIBindGroup> VulkanResourceFactory::CreateBindGroupImpl(
        const RHIBindGroupDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto bindGroup = std::make_shared<VulkanBindGroup>();
        if (!bindGroup->Create(dev, m_DescriptorPool, desc))
        {
            return nullptr;
        }
        return bindGroup;
    }

    // CreateGraphicsPipelineImpl — moved from VulkanDevice.cpp lines 672–681.
    std::shared_ptr<RHIPipeline> VulkanResourceFactory::CreateGraphicsPipelineImpl(
        const RHIGraphicsPipelineDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto pipeline = std::make_shared<VulkanPipeline>(dev, desc);
        if (!pipeline->IsValid())
        {
            return nullptr;
        }
        return pipeline;
    }
}