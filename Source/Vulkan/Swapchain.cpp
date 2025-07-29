#include "Swapchain.h"
#include "VkBase.h"

namespace VK
{
	result_t Swapchain::Build(bool limitFrameRate, VkSwapchainCreateFlagsKHR flags)
	{
        PhysicalDevice& physicalDevice = VkBase::Base().PhysicalDevice();
        VkSurfaceKHR surface = VkBase::Base().Surface();

        const VkSurfaceCapabilitiesKHR& surfaceCapabilities = physicalDevice.SurfaceCapabilities();
        const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats = physicalDevice.AvailableSurfaceFormats();
        const std::vector<VkPresentModeKHR>& surfacePresentModes = physicalDevice.SurfacePresentModes();

        swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
        swapchainCreateInfo.imageExtent =
            surfaceCapabilities.currentExtent.width == -1 ?
            VkExtent2D{
                glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
                glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height) } :
                surfaceCapabilities.currentExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;

        if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        {
            swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        }
        else
        {
            for (size_t i = 0; i < 4; ++i)
            {
                if (surfaceCapabilities.supportedCompositeAlpha & 1 << i)
                {
                    swapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
                    break;
                }
            }
        }

        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        {
            swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        {
            swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        else
        {
            std::cout << std::format("[ VkBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n");
        }

        
        if (!swapchainCreateInfo.imageFormat)
        {
            if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }, availableSurfaceFormats) &&
                SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }, availableSurfaceFormats))
            {
                swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
                swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
                std::cout << std::format("[ VkBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
            }
        }
   
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        if (!limitFrameRate)
        {
            for (size_t i = 0; i < surfacePresentModes.size(); i++)
            {
                if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
            }
        }

        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.flags = flags;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.clipped = VK_TRUE;

        if (VkResult result = Build_Internal())
        {
            return result;
        }

        for (auto& callback : callbacks_onCreate)
        {
            callback();
        }

        return VK_SUCCESS;
	}

    result_t Swapchain::ReBuild()
    {
        PhysicalDevice& physicalDevice = VkBase::Base().PhysicalDevice();
        LogicalDevice& device = VkBase::Base().Device();

        const VkSurfaceCapabilitiesKHR& surfaceCapabilities = physicalDevice.SurfaceCapabilities();
        if (surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0)
        {
            return VK_SUBOPTIMAL_KHR;
        }
        swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
        swapchainCreateInfo.oldSwapchain = handle;

        VkResult result = vkQueueWaitIdle(device.Queue_Graphics());
        
        if (result == VK_SUCCESS && device.Queue_Graphics() != device.Queue_Presentation())
        {
            result = vkQueueWaitIdle(device.Queue_Presentation());
        }
        if (result)
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
            return result;
        }

        for (auto& callback : callbacks_onDestroy)
        {
            callback();
        }

        for (VkImageView& imageView : swapchainImageViews)
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
        swapchainImageViews.resize(0);

        if (result = Build_Internal())
        {
            return result;
        }

        for (const auto& callback : callbacks_onCreate)
        {
            callback();
        }

        return VK_SUCCESS;
    }

    void Swapchain::Destroy()
    {
        for (auto& callback : callbacks_onDestroy) callback();

        for (VkImageView& imageView : swapchainImageViews)
        {
            if (imageView) vkDestroyImageView(VkBase::Base().Device(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(VkBase::Base().Device(), handle, nullptr);

        handle = VK_NULL_HANDLE;
    }

    result_t Swapchain::SwapImage(VkSemaphore semaphore_imageIsAvailable)
    {
        if (swapchainCreateInfo.oldSwapchain &&
            swapchainCreateInfo.oldSwapchain != handle)
        {
			vkDestroySwapchainKHR(VkBase::Base().Device(), swapchainCreateInfo.oldSwapchain, nullptr);
            swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        }

		while (VkResult result = vkAcquireNextImageKHR(VkBase::Base().Device(), handle, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex))
		{
			switch (result)
			{
			case VK_SUBOPTIMAL_KHR:

			case VK_ERROR_OUT_OF_DATE_KHR:
				if (VkResult result = ReBuild()) return result;
				break; 
                // Note that after the swapchain is reconstructed, 
                // it is still necessary to obtain the image. 
                // Through the break recursion, the condition judgment statement of the while loop is executed again.

			default:
				outStream << std::format("[ VkBase ] ERROR\nFailed to acquire the next image!\nError code: {}\n", int32_t(result));
				return result;
			}
		}

		return VK_SUCCESS;
    }

    result_t Swapchain::PresentImage(VkPresentInfoKHR& presentInfo)
    {
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		switch (VkResult result = vkQueuePresentKHR(VkBase::Base().Device().Queue_Presentation(), &presentInfo))
		{
		case VK_SUCCESS:
			return VK_SUCCESS;
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			return ReBuild();
		default:
			outStream << std::format("[ VkBase ] ERROR\nFailed to queue the image for presentation!\nError code: {}\n", int32_t(result));
			return result;
		}
    }

    result_t Swapchain::PresentImage(VkSemaphore semaphore_renderingIsOver)
    {
		VkPresentInfoKHR presentInfo = {
		.swapchainCount = 1,
		.pSwapchains = &handle,
		.pImageIndices = &currentImageIndex
		};

		if (semaphore_renderingIsOver)
		{
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &semaphore_renderingIsOver;
		}

		return PresentImage(presentInfo);
    }

	result_t Swapchain::Build_Internal()
	{
        LogicalDevice& device = VkBase::Base().Device();

        if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &handle))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
            return result;
        }

        uint32_t swapchainImageCount;
        if (VkResult result = vkGetSwapchainImagesKHR(device, handle, &swapchainImageCount, nullptr))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
            return result;
        }

        swapchainImages.resize(swapchainImageCount);
        if (VkResult result = vkGetSwapchainImagesKHR(device, handle, &swapchainImageCount, swapchainImages.data()))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
            return result;
        }

        swapchainImageViews.resize(swapchainImageCount);
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
        imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        for (size_t i = 0; i < swapchainImageCount; ++i)
        {
            imageViewCreateInfo.image = swapchainImages[i];
            if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]))
            {
                std::cout << std::format("[ VkBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
                return result;
            }
        }
        return VK_SUCCESS;
	}

    result_t Swapchain::SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat, const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats)
    {
        bool formatIsAvailable = false;
        if (!surfaceFormat.format)
        {
            // if does not specify format, only match the color space, format use what we have
            for (const VkSurfaceFormatKHR& i : availableSurfaceFormats)
            {
                if (i.colorSpace == surfaceFormat.colorSpace)
                {
                    swapchainCreateInfo.imageFormat = i.format;
                    swapchainCreateInfo.imageColorSpace = i.colorSpace;
                    formatIsAvailable = true;
                    break;
                }
            }
        }
        else
        {
            // otherwise both need to match
            for (auto& i : availableSurfaceFormats)
            {
                if (i.format == surfaceFormat.format && i.colorSpace == surfaceFormat.colorSpace)
                {
                    swapchainCreateInfo.imageFormat = i.format;
                    swapchainCreateInfo.imageColorSpace = i.colorSpace;
                    formatIsAvailable = true;
                    break;
                }
            }
        }

        if (!formatIsAvailable)
        {
            return VK_ERROR_FORMAT_NOT_SUPPORTED;
        }
       
        // If swapchain already exist, rebuild
        if (handle)
        {
            return ReBuild();
        }

        return VK_SUCCESS;
    }
}