#include "VulkanTexture.h"

#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    namespace
    {
        VkImageUsageFlags ToVulkanTextureUsage(RHITextureUsage usage)
        {
            VkImageUsageFlags flags = 0;
            if (HasFlag(usage, RHITextureUsage::ColorAttachment)) { flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; }
            if (HasFlag(usage, RHITextureUsage::DepthStencil)) { flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }
            if (HasFlag(usage, RHITextureUsage::Sampled)) { flags |= VK_IMAGE_USAGE_SAMPLED_BIT; }
            if (HasFlag(usage, RHITextureUsage::TransferSrc)) { flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; }
            if (HasFlag(usage, RHITextureUsage::TransferDst)) { flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; }
            return flags;
        }

        VkImageAspectFlags GetAspectMask(RHIFormat format)
        {
            return format == RHIFormat::D32Float ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    VulkanTexture::VulkanTexture(VkDevice device, VmaAllocator allocator, const RHITextureDesc& desc)
        : m_Device(device)
        , m_Allocator(allocator)
        , m_Width(desc.Width)
        , m_Height(desc.Height)
        , m_Format(desc.Format)
    {
        if (m_Device == VK_NULL_HANDLE || m_Allocator == VK_NULL_HANDLE || desc.Width == 0 || desc.Height == 0)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture with invalid device, allocator, or extent");
            return;
        }

        const VkFormat format = RHIFormatToVulkanFormat(desc.Format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture with unsupported format");
            return;
        }

        VkImageCreateInfo imageCreateInfo {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent = { desc.Width, desc.Height, 1 };
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = ToVulkanTextureUsage(desc.Usage);
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkResult result = vmaCreateImage(
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
            return;
        }

        VkImageViewCreateInfo viewCreateInfo {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = m_Image;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = format;
        viewCreateInfo.subresourceRange.aspectMask = GetAspectMask(desc.Format);
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(m_Device, &viewCreateInfo, nullptr, &m_ImageView);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan image view: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
        }
    }

    VulkanTexture::~VulkanTexture()
    {
        if (m_Device != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_ImageView, nullptr);
            m_ImageView = VK_NULL_HANDLE;
        }

        if (m_Allocator != VK_NULL_HANDLE && m_Image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
            m_Image = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
        }
    }

    bool VulkanTexture::IsValid() const { return m_Image != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE; }
    u32 VulkanTexture::GetWidth() const { return m_Width; }
    u32 VulkanTexture::GetHeight() const { return m_Height; }
    RHIFormat VulkanTexture::GetFormat() const { return m_Format; }
    VkImage VulkanTexture::GetImage() const { return m_Image; }
    VkImageView VulkanTexture::GetImageView() const { return m_ImageView; }
    VkImageLayout* VulkanTexture::GetLayoutPtr() { return &m_Layout; }
}
