#include "VulkanTextureView.h"

#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"
#include "VulkanCheckedCast.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <string>

namespace XEngine
{
    namespace
    {
        VkImageViewType ToVulkanImageViewType(RHITextureViewDimension dim)
        {
            switch (dim)
            {
            case RHITextureViewDimension::Texture2D:
                return VK_IMAGE_VIEW_TYPE_2D;
            case RHITextureViewDimension::Texture2DArray:
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case RHITextureViewDimension::TextureCube:
                return VK_IMAGE_VIEW_TYPE_CUBE;
            }
            return VK_IMAGE_VIEW_TYPE_2D;
        }

        VkImageAspectFlags ToVulkanAspectFlags(RHITextureAspectFlags flags)
        {
            VkImageAspectFlags result = 0;
            if (HasFlag(flags, RHITextureAspectFlags::Depth))
            {
                result |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (HasFlag(flags, RHITextureAspectFlags::Stencil))
            {
                result |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            if (result == 0)
            {
                result = VK_IMAGE_ASPECT_COLOR_BIT;
            }
            return result;
        }
    }

    VulkanTextureView::VulkanTextureView(VulkanDevice& device, const RHITextureViewDesc& desc)
        : RHITextureView(device)
        , m_Device(device.GetHandle())
        , m_Desc(desc)
    {
        XENGINE_ASSERT(desc.Texture != nullptr, "VulkanTextureView requires a valid device and source texture");

        const RHITextureDesc& texDesc = desc.Texture->GetDesc();
        const VkFormat format = RHIFormatToVulkanFormat(texDesc.Format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture view: unsupported format");
            return;
        }

        const u32 mipCount = (desc.MipCount == 0) ?
            (texDesc.MipLevels - desc.BaseMipLevel) : desc.MipCount;
        const u32 layerCount = (desc.ArrayLayerCount == 0) ?
            (texDesc.ArrayLayers - desc.BaseArrayLayer) : desc.ArrayLayerCount;

        const VulkanTexture* sourceTexture = CheckedVulkanCast<VulkanTexture>(desc.Texture, device);

        VkImageViewCreateInfo viewInfo {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = sourceTexture->GetImage();
        viewInfo.viewType = ToVulkanImageViewType(desc.ViewDimension);
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = ToVulkanAspectFlags(desc.Aspect);
        viewInfo.subresourceRange.baseMipLevel = desc.BaseMipLevel;
        viewInfo.subresourceRange.levelCount = mipCount;
        viewInfo.subresourceRange.baseArrayLayer = desc.BaseArrayLayer;
        viewInfo.subresourceRange.layerCount = layerCount;

        XENGINE_VK_CHECK(vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView));
    }

    VulkanTextureView::~VulkanTextureView()
    {
        if (m_Device != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_ImageView, nullptr);
            m_ImageView = VK_NULL_HANDLE;
        }
        m_Device = VK_NULL_HANDLE;
        m_Desc = {};
    }

    bool VulkanTextureView::IsValid() const
    {
        return m_ImageView != VK_NULL_HANDLE;
    }

    const RHITextureViewDesc& VulkanTextureView::GetDesc() const
    {
        return m_Desc;
    }

    VkImageView VulkanTextureView::GetHandle() const
    {
        return m_ImageView;
    }

    void* VulkanTextureView::GetNativeView(RHIBackend backend) const
    {
        return backend == RHIBackend::Vulkan
            ? reinterpret_cast<void*>(m_ImageView)
            : nullptr;
    }
}