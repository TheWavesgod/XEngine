#include "VulkanTexture.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    VulkanTexture::VulkanTexture(
        VulkanDevice& device,
        VmaAllocator allocator,
        const RHITextureDesc& desc)
        : RHITexture(device)
        , m_Device(device.GetHandle())
        , m_Allocator(allocator)
        , m_Desc(desc)
    {
        if (m_Device == VK_NULL_HANDLE || m_Allocator == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture without a device and allocator");
            return;
        }

        const VkFormat format = RHIFormatToVulkanFormat(m_Desc.Format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture with unsupported format");
            return;
        }

        VkImageCreateInfo imageCreateInfo {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.flags = m_Desc.Dimension == RHITextureDimension::TextureCube
            ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
            : 0;
        imageCreateInfo.imageType = ToVulkanImageType(m_Desc.Dimension);
        imageCreateInfo.extent = { m_Desc.Width, m_Desc.Height, 1 };
        imageCreateInfo.mipLevels = m_Desc.MipLevels;
        imageCreateInfo.arrayLayers = m_Desc.ArrayLayers;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = ToVulkanImageUsageFlags(m_Desc.Usage);
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        const VkResult result = vmaCreateImage(
            m_Allocator,
            &imageCreateInfo,
            &allocationCreateInfo,
            &m_Image,
            &m_Allocation,
            &m_AllocationInfo);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan image: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
        }
    }

    VulkanTexture::~VulkanTexture()
    {
        if (m_Allocator != VK_NULL_HANDLE && m_Image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
            m_Image = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
        }
    }

    bool VulkanTexture::IsValid() const
    {
        return m_Image != VK_NULL_HANDLE;
    }

    const RHITextureDesc& VulkanTexture::GetDesc() const
    {
        return m_Desc;
    }

    VkImage VulkanTexture::GetImage() const
    {
        return m_Image;
    }

    VkImageLayout* VulkanTexture::GetLayoutPtr()
    {
        return &m_Layout;
    }
}
