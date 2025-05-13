#pragma once

#include "VKEasyHearder.h"

namespace VK
{
	class PhysicalDevice;
	class LogicalDevice;

	class Swapchain
	{
		VkSwapchainKHR handle;

		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;

		// In order to rebuild the swapchain conveniently, save the createInfo of swapchain 
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

		PhysicalDevice* p_physicalDevice = nullptr;
		LogicalDevice* p_device = nullptr;

		uint32_t currentImageIndex = 0;

	public:
		Swapchain() = default;

		/*
		 * Function used to build the swapchain 
		 */
		result_t Build(PhysicalDevice& physicalDevice, LogicalDevice& device, bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);

		result_t ReBuild();

		void Destroy();

		result_t SwapImage(VkSemaphore semaphore_imageIsAvailable);

		result_t PresentImage(VkPresentInfoKHR& presentInfo);

		result_t PresentImage(VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE);

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		VkSwapchainKHR& Ref() { return handle; }

		VkSwapchainCreateInfoKHR& SwapchainCreateInfo() { return swapchainCreateInfo; }

		const std::vector<VkImage>& Images() const { return swapchainImages; }
		std::vector<VkImageView>& ImageViews() { return swapchainImageViews; } 

		uint32_t CurrentImageIndex() const { return currentImageIndex; }

	private:
		result_t Build_Internal();

		result_t SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat, const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats);
	};
}


