#include "VulkanSampler.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    VulkanSampler::VulkanSampler(VulkanDevice& device, const RHISamplerDesc& desc)
        : RHISampler(device)
        , m_Device(device.GetHandle())
        , m_Desc(desc)
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan sampler with invalid device");
            return;
        }

        VkSamplerCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = ToVulkanFilter(m_Desc.MagFilter);
        createInfo.minFilter = ToVulkanFilter(m_Desc.MinFilter);
        createInfo.addressModeU = ToVulkanAddressMode(m_Desc.AddressU);
        createInfo.addressModeV = ToVulkanAddressMode(m_Desc.AddressV);
        createInfo.addressModeW = ToVulkanAddressMode(m_Desc.AddressW);
        createInfo.anisotropyEnable = VK_FALSE;
        createInfo.maxAnisotropy = 1.0f;
        createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.compareEnable = VK_FALSE;
        createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.mipLodBias = 0.0f;
        createInfo.minLod = 0.0f;
        createInfo.maxLod = VK_LOD_CLAMP_NONE;

        if (m_Desc.MaxAnisotropy > 1.0f)
        {
            XENGINE_LOG_WARN("Sampler anisotropy feature query is not wired in Stage 6A. Creating sampler without anisotropy.");
        }

        const VkResult result = vkCreateSampler(m_Device, &createInfo, nullptr, &m_Sampler);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan sampler: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
        }
    }

    VulkanSampler::~VulkanSampler()
    {
        if (m_Device != VK_NULL_HANDLE && m_Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }
    }

    bool VulkanSampler::IsValid() const
    {
        return m_Sampler != VK_NULL_HANDLE;
    }

    const RHISamplerDesc& VulkanSampler::GetDesc() const
    {
        return m_Desc;
    }

    VkSampler VulkanSampler::GetHandle() const
    {
        return m_Sampler;
    }

    void* VulkanSampler::GetNativeSampler(RHIBackend backend) const
    {
        return backend == RHIBackend::Vulkan ? m_Sampler : nullptr;
    }
}
