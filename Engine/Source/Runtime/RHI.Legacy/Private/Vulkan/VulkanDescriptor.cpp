#include "VulkanDescriptor.h"

#include "VulkanBuffer.h"
#include "VulkanCheckedCast.h"
#include "VulkanDevice.h"
#include "VulkanSampler.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/Core/Assert.h>

#include <string>
#include <vector>

namespace XEngine
{
    VulkanBindGroupLayout::VulkanBindGroupLayout(VulkanDevice& device)
        : RHIBindGroupLayout(device)
    {
    }

    VulkanBindGroupLayout::~VulkanBindGroupLayout()
    {
        Destroy();
    }

    bool VulkanBindGroupLayout::Create(VulkanDevice& device, const RHIBindGroupLayoutDesc& desc)
    {
        m_Device = device.GetHandle();
        if (m_Device == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan bind group layout without a valid device");
            return false;
        }

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(desc.Entries.size());

        for (const RHIBindGroupLayoutEntry& entry : desc.Entries)
        {
            const VkDescriptorType descriptorType = ToVulkanDescriptorType(entry.Type);
            if (descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
            {
                XENGINE_LOG_ERROR("Unsupported bind group layout binding type");
                return false;
            }

            VkDescriptorSetLayoutBinding binding {};
            binding.binding = entry.Binding;
            binding.descriptorType = descriptorType;
            binding.descriptorCount = entry.Count;
            binding.stageFlags = ToVulkanShaderStageFlags(entry.Visibility);
            bindings.push_back(binding);
        }

        VkDescriptorSetLayoutCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<u32>(bindings.size());
        createInfo.pBindings = bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(m_Device, &createInfo, nullptr, &m_Layout);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan descriptor set layout: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        m_Desc = desc;
        XENGINE_LOG_INFO(desc.DebugName != nullptr ? desc.DebugName : "Vulkan descriptor set layout created");
        return true;
    }

    void VulkanBindGroupLayout::Destroy()
    {
        if (m_Device != VK_NULL_HANDLE && m_Layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
            m_Layout = VK_NULL_HANDLE;
        }
    }

    VkDescriptorSetLayout VulkanBindGroupLayout::GetHandle() const
    {
        return m_Layout;
    }

    const RHIBindGroupLayoutDesc& VulkanBindGroupLayout::GetDesc() const
    {
        return m_Desc;
    }

    VulkanBindGroup::~VulkanBindGroup()
    {
        Destroy();
    }

    VulkanBindGroup::VulkanBindGroup(VulkanDevice& device)
        : RHIBindGroup(device)
    {
    }

    bool VulkanBindGroup::Create(
        VulkanDevice& device,
        VkDescriptorPool descriptorPool,
        const RHIBindGroupDesc& desc)
    {
        m_Device = device.GetHandle();
        if (m_Device == VK_NULL_HANDLE || descriptorPool == VK_NULL_HANDLE || desc.Layout == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan bind group without device, descriptor pool, and layout");
            return false;
        }

        auto* layout = CheckedVulkanCast<VulkanBindGroupLayout>(desc.Layout, device);
        if (layout == nullptr || layout->GetHandle() == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Vulkan bind group requires a valid Vulkan bind group layout");
            return false;
        }

        m_DescriptorPool = descriptorPool;

        VkDescriptorSetLayout vkLayout = layout->GetHandle();
        VkDescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = m_DescriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &vkLayout;

        VkResult result = vkAllocateDescriptorSets(m_Device, &allocateInfo, &m_Set);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to allocate Vulkan descriptor set: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkWriteDescriptorSet> writes;
        imageInfos.reserve(desc.Resources.size());
        bufferInfos.reserve(desc.Resources.size());
        writes.reserve(desc.Resources.size());

        for (const RHIBindingResource& resource : desc.Resources)
        {
            if (resource.Type == RHIBindingType::CombinedImageSampler ||
                resource.Type == RHIBindingType::SampledTexture ||
                resource.Type == RHIBindingType::Sampler)
            {
                VulkanTextureView* view = resource.TextureView != nullptr
                    ? CheckedVulkanCast<VulkanTextureView>(resource.TextureView, device)
                    : nullptr;
                VulkanSampler* sampler = resource.Sampler != nullptr
                    ? CheckedVulkanCast<VulkanSampler>(resource.Sampler, device)
                    : nullptr;
                if ((view != nullptr && view->GetHandle() == VK_NULL_HANDLE) ||
                    (sampler != nullptr && sampler->GetHandle() == VK_NULL_HANDLE))
                {
                    XENGINE_LOG_ERROR("Image binding requires valid Vulkan resources");
                    return false;
                }

                VkDescriptorImageInfo imageInfo {};
                imageInfo.imageLayout = view != nullptr
                    ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    : VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.imageView = view != nullptr ? view->GetHandle() : VK_NULL_HANDLE;
                imageInfo.sampler = sampler != nullptr ? sampler->GetHandle() : VK_NULL_HANDLE;
                imageInfos.push_back(imageInfo);

                VkWriteDescriptorSet write {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = m_Set;
                write.dstBinding = resource.Binding;
                write.descriptorCount = 1;
                write.descriptorType = ToVulkanDescriptorType(resource.Type);
                write.pImageInfo = &imageInfos.back();
                writes.push_back(write);
                continue;
            }

            if (resource.Type == RHIBindingType::UniformBuffer || resource.Type == RHIBindingType::StorageBuffer)
            {
                XENGINE_ASSERT(resource.Buffer != nullptr, "Buffer binding requires a non-null RHIBuffer");
                auto* buffer = CheckedVulkanCast<VulkanBuffer>(resource.Buffer, device);
                if (buffer == nullptr || buffer->GetHandle() == VK_NULL_HANDLE)
                {
                    XENGINE_LOG_ERROR("Buffer binding requires a valid Vulkan buffer");
                    return false;
                }

                VkDescriptorBufferInfo bufferInfo {};
                bufferInfo.buffer = buffer->GetHandle();
                bufferInfo.offset = resource.BufferOffset;
                bufferInfo.range = resource.BufferSize == 0
                    ? buffer->GetSize() - resource.BufferOffset
                    : resource.BufferSize;
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet write {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = m_Set;
                write.dstBinding = resource.Binding;
                write.descriptorCount = 1;
                write.descriptorType = ToVulkanDescriptorType(resource.Type);
                write.pBufferInfo = &bufferInfos.back();
                writes.push_back(write);
                continue;
            }

            XENGINE_LOG_ERROR("Unsupported Vulkan bind group resource type");
            return false;
        }

        vkUpdateDescriptorSets(m_Device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);

        m_Desc = desc;
        XENGINE_LOG_INFO(desc.DebugName != nullptr ? desc.DebugName : "Vulkan descriptor set created");
        return true;
    }

    void VulkanBindGroup::Destroy()
    {
        if (m_Device != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE && m_Set != VK_NULL_HANDLE)
        {
            vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_Set);
            m_Set = VK_NULL_HANDLE;
        }
    }

    VkDescriptorSet VulkanBindGroup::GetHandle() const
    {
        return m_Set;
    }

    const RHIBindGroupDesc& VulkanBindGroup::GetDesc() const
    {
        return m_Desc;
    }
}
