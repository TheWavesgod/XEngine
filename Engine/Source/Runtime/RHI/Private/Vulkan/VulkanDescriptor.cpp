#include "VulkanDescriptor.h"

#include "VulkanSampler.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <string>
#include <vector>

namespace XEngine
{
    VulkanBindGroupLayout::~VulkanBindGroupLayout()
    {
        Destroy();
    }

    bool VulkanBindGroupLayout::Create(VkDevice device, const RHIBindGroupLayoutDesc& desc)
    {
        if (device == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan bind group layout without a valid device");
            return false;
        }

        m_Device = device;

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

    bool VulkanBindGroup::Create(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        const RHIBindGroupDesc& desc)
    {
        if (device == VK_NULL_HANDLE || descriptorPool == VK_NULL_HANDLE || desc.Layout == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan bind group without device, descriptor pool, and layout");
            return false;
        }

        auto* layout = dynamic_cast<VulkanBindGroupLayout*>(desc.Layout);
        if (layout == nullptr || layout->GetHandle() == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Vulkan bind group requires a valid Vulkan bind group layout");
            return false;
        }

        m_Device = device;
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
        std::vector<VkWriteDescriptorSet> writes;
        imageInfos.reserve(desc.Resources.size());
        writes.reserve(desc.Resources.size());

        for (const RHIBindingResource& resource : desc.Resources)
        {
            if (resource.Type != RHIBindingType::CombinedImageSampler)
            {
                XENGINE_LOG_ERROR("VulkanBindGroup currently supports combined image sampler resources only");
                return false;
            }

            auto* texture = dynamic_cast<VulkanTexture*>(resource.Texture);
            auto* sampler = dynamic_cast<VulkanSampler*>(resource.Sampler);
            if (texture == nullptr || sampler == nullptr ||
                texture->GetImageView() == VK_NULL_HANDLE || sampler->GetHandle() == VK_NULL_HANDLE)
            {
                XENGINE_LOG_ERROR("Combined image sampler binding requires a valid Vulkan texture and sampler");
                return false;
            }

            VkDescriptorImageInfo imageInfo {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture->GetImageView();
            imageInfo.sampler = sampler->GetHandle();
            imageInfos.push_back(imageInfo);

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_Set;
            write.dstBinding = resource.Binding;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &imageInfos.back();
            writes.push_back(write);
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
