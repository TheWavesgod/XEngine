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
    VulkanTextureView::VulkanTextureView(VulkanDevice& device, const RHITextureViewDesc& desc)
        : RHITextureView(device)
        , m_Device(device.GetHandle())
        , m_Desc(desc)
    {
        XENGINE_ASSERT(desc.Texture != nullptr, "VulkanTextureView requires a valid device and source texture");

        const VkFormat format = RHIFormatToVulkanFormat(desc.Format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture view: unsupported format");
            return;
        }

        const VulkanTexture* sourceTexture = CheckedVulkanCast<VulkanTexture>(desc.Texture, device);

        VkImageViewCreateInfo viewInfo {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = sourceTexture->GetImage();
        viewInfo.viewType = ToVulkanImageViewType(desc.ViewDimension);
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = ToVulkanImageAspectFlags(desc.Aspect);
        viewInfo.subresourceRange.baseMipLevel = desc.BaseMipLevel;
        viewInfo.subresourceRange.levelCount = desc.MipCount;
        viewInfo.subresourceRange.baseArrayLayer = desc.BaseArrayLayer;
        viewInfo.subresourceRange.layerCount = desc.ArrayLayerCount;

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

}
