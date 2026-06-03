#include "VulkanSwapchain.h"

#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <algorithm>
#include <limits>
#include <string>

namespace XEngine
{
    namespace
    {
        VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
        {
            for (const VkSurfaceFormatKHR& format : formats)
            {
                if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return format;
                }
            }

            return formats.front();
        }

        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes, bool enableVSync)
        {
            if (enableVSync)
            {
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            for (VkPresentModeKHR presentMode : presentModes)
            {
                if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    return presentMode;
                }
            }

            for (VkPresentModeKHR presentMode : presentModes)
            {
                if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    return presentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, u32 width, u32 height)
        {
            if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
            {
                return capabilities.currentExtent;
            }

            VkExtent2D extent {};
            extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return extent;
        }
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        Destroy();
    }

    bool VulkanSwapchain::Create(const VulkanSwapchainCreateInfo& createInfo)
    {
        m_Device = createInfo.Device;

        VkSurfaceCapabilitiesKHR capabilities {};
        XENGINE_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            createInfo.PhysicalDevice, createInfo.Surface, &capabilities));

        u32 formatCount = 0;
        XENGINE_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            createInfo.PhysicalDevice, createInfo.Surface, &formatCount, nullptr));
        if (formatCount == 0)
        {
            XENGINE_LOG_ERROR("Vulkan surface has no supported formats");
            return false;
        }

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        XENGINE_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            createInfo.PhysicalDevice, createInfo.Surface, &formatCount, formats.data()));

        u32 presentModeCount = 0;
        XENGINE_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            createInfo.PhysicalDevice, createInfo.Surface, &presentModeCount, nullptr));
        if (presentModeCount == 0)
        {
            XENGINE_LOG_ERROR("Vulkan surface has no supported present modes");
            return false;
        }

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        XENGINE_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            createInfo.PhysicalDevice, createInfo.Surface, &presentModeCount, presentModes.data()));

        const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(formats);
        const VkPresentModeKHR presentMode = ChoosePresentMode(presentModes, createInfo.EnableVSync);
        const VkExtent2D extent = ChooseExtent(capabilities, createInfo.Width, createInfo.Height);

        u32 imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
        {
            imageCount = std::min(imageCount, capabilities.maxImageCount);
        }

        VkSwapchainCreateInfoKHR swapchainCreateInfo {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = createInfo.Surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.preTransform = capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        const u32 queueFamilyIndices[] = {
            createInfo.GraphicsQueueFamilyIndex,
            createInfo.PresentQueueFamilyIndex
        };

        if (createInfo.GraphicsQueueFamilyIndex != createInfo.PresentQueueFamilyIndex)
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VkResult result = vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, nullptr, &m_Swapchain);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan swapchain: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        m_ImageFormat = surfaceFormat.format;
        m_ColorSpace = surfaceFormat.colorSpace;
        m_Extent = extent;

        XENGINE_VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr));
        m_Images.resize(imageCount);
        XENGINE_VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_Images.data()));

        m_ImageViews.resize(m_Images.size());
        for (u32 index = 0; index < static_cast<u32>(m_Images.size()); ++index)
        {
            VkImageViewCreateInfo imageViewCreateInfo {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = m_Images[index];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = m_ImageFormat;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            result = vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &m_ImageViews[index]);
            if (result != VK_SUCCESS)
            {
                XENGINE_LOG_ERROR("Failed to create Vulkan swapchain image view");
                Destroy();
                return false;
            }
        }

        std::string message = "Vulkan swapchain created: ";
        message += std::to_string(m_Extent.width);
        message += "x";
        message += std::to_string(m_Extent.height);
        XENGINE_LOG_INFO(message);
        return true;
    }

    void VulkanSwapchain::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        for (VkImageView imageView : m_ImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, imageView, nullptr);
            }
        }
        m_ImageViews.clear();
        m_Images.clear();

        if (m_Swapchain != VK_NULL_HANDLE)
        {
            XENGINE_LOG_INFO("Destroying Vulkan swapchain");
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }

        m_ImageFormat = VK_FORMAT_UNDEFINED;
        m_Extent = {};
        m_Device = VK_NULL_HANDLE;
    }

    bool VulkanSwapchain::Recreate(const VulkanSwapchainCreateInfo& createInfo)
    {
        Destroy();
        return Create(createInfo);
    }

    VkSwapchainKHR VulkanSwapchain::GetHandle() const
    {
        return m_Swapchain;
    }

    VkFormat VulkanSwapchain::GetImageFormat() const
    {
        return m_ImageFormat;
    }

    VkExtent2D VulkanSwapchain::GetExtent() const
    {
        return m_Extent;
    }

    u32 VulkanSwapchain::GetImageCount() const
    {
        return static_cast<u32>(m_Images.size());
    }

    VkImage VulkanSwapchain::GetImage(u32 index) const
    {
        return m_Images[index];
    }

    VkImageView VulkanSwapchain::GetImageView(u32 index) const
    {
        return m_ImageViews[index];
    }
}
