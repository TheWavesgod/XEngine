#include "VulkanTexture.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    namespace
    {
        VkImageAspectFlags GetAspectMask(RHIFormat format)
        {
            return format == RHIFormat::D32Float ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkImageViewType GetImageViewType(RHITextureDimension dimension)
        {
            switch (dimension)
            {
            case RHITextureDimension::TextureCube:
                return VK_IMAGE_VIEW_TYPE_CUBE;
            case RHITextureDimension::Texture2D:
            default:
                return VK_IMAGE_VIEW_TYPE_2D;
            }
        }
    }

    VulkanTexture::VulkanTexture(VulkanDevice& device, VmaAllocator allocator, const RHITextureDesc& desc)
        : RHITexture(device)
        , m_Device(device.GetHandle())
        , m_Allocator(allocator)
        , m_Desc(desc)
    {
        if (m_Desc.GenerateMips)
        {
            XENGINE_LOG_WARN("Texture mip generation is not implemented in Stage 6A. Creating the base mip only.");
            m_Desc.MipLevels = 1;
            m_Desc.GenerateMips = false;
        }

        if (m_Device == VK_NULL_HANDLE || m_Allocator == VK_NULL_HANDLE || m_Desc.Width == 0 || m_Desc.Height == 0)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture with invalid device, allocator, or extent");
            return;
        }

        if (m_Desc.Dimension == RHITextureDimension::TextureCube && m_Desc.ArrayLayers != 6)
        {
            XENGINE_LOG_ERROR("TextureCube requires exactly 6 array layers");
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
        imageCreateInfo.flags = m_Desc.Dimension == RHITextureDimension::TextureCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
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
    }

    VulkanTexture::~VulkanTexture()
    {
        // Drop the default view first — its VkImageView depends on this image.
        m_DefaultView.reset();

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

    RHITextureView* VulkanTexture::GetDefaultView() const
    {
        if (!m_DefaultView)
        {
            RHITextureViewDesc viewDesc;
            viewDesc.Texture = this;
            viewDesc.Usage = RHITextureViewUsageFlags::Sampled;
            viewDesc.Aspect = m_Desc.Format == RHIFormat::D32Float ? 
                RHITextureAspectFlags::Depth : RHITextureAspectFlags::Color;
            viewDesc.BaseMipLevel = 0;
            viewDesc.MipCount = m_Desc.MipLevels;
            viewDesc.BaseArrayLayer = 0;
            viewDesc.ArrayLayerCount = m_Desc.ArrayLayers;
            viewDesc.DebugName = m_Desc.DebugName;

            m_DefaultView = static_cast<VulkanDevice&>(GetOwnerDevice())
                                .CreateTextureView(viewDesc);
        }
        return m_DefaultView.get();
    }

    void* VulkanTexture::GetNativeDefaultView(RHIBackend backend) const
    {
        return backend == RHIBackend::Vulkan ? m_DefaultView->GetNativeView(backend) : nullptr;
    }

    VkImageLayout* VulkanTexture::GetLayoutPtr()
    {
        return &m_Layout;
    }
}
