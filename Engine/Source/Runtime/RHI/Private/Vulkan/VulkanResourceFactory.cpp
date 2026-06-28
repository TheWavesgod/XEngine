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
        , m_OwnerDevice(ownerDevice)
        , m_Allocator(ownerDevice.GetVmaAllocator())
        , m_DescriptorPool(ownerDevice.GetDescriptorPool())
    {
    }

    // CreateBufferImpl
    std::shared_ptr<RHIBuffer> VulkanResourceFactory::CreateBufferImpl(
        const RHIBufferDesc& desc)
    {
        auto buffer = std::make_shared<VulkanBuffer>(m_OwnerDevice, m_Allocator, desc);
        if (!buffer->IsValid())
        {
            return nullptr;
        }
        return buffer;
    }

    // CreateTextureImpl
    std::shared_ptr<RHITexture> VulkanResourceFactory::CreateTextureImpl(
        const RHITextureDesc& desc)
    {
        auto texture = std::make_shared<VulkanTexture>(m_OwnerDevice, m_Allocator, desc);
        if (!texture->IsValid())
        {
            return nullptr;
        }

        return texture;
    }

    // CreateTextureViewImpl 
    std::shared_ptr<RHITextureView> VulkanResourceFactory::CreateTextureViewImpl(
        const RHITextureViewDesc& desc)
    {
        auto view = std::make_shared<VulkanTextureView>(m_OwnerDevice, desc);
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
        auto sampler = std::make_shared<VulkanSampler>(m_OwnerDevice, desc);
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
        auto shader = std::make_shared<VulkanShader>(m_OwnerDevice, desc);
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
        auto layout = std::make_shared<VulkanBindGroupLayout>(m_OwnerDevice);
        if (!layout->Create(m_OwnerDevice, desc))
        {
            return nullptr;
        }
        return layout;
    }

    // CreateBindGroupImpl — moved from VulkanDevice.cpp lines 661–670.
    std::shared_ptr<RHIBindGroup> VulkanResourceFactory::CreateBindGroupImpl(
        const RHIBindGroupDesc& desc)
    {
        auto bindGroup = std::make_shared<VulkanBindGroup>(m_OwnerDevice);
        if (!bindGroup->Create(m_OwnerDevice, m_DescriptorPool, desc))
        {
            return nullptr;
        }
        return bindGroup;
    }

    // CreateGraphicsPipelineImpl — moved from VulkanDevice.cpp lines 672–681.
    std::shared_ptr<RHIPipeline> VulkanResourceFactory::CreateGraphicsPipelineImpl(
        const RHIGraphicsPipelineDesc& desc)
    {
        auto pipeline = std::make_shared<VulkanPipeline>(m_OwnerDevice, desc);
        if (!pipeline->IsValid())
        {
            return nullptr;
        }
        return pipeline;
    }
}
