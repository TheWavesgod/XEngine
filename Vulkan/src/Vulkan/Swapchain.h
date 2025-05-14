#pragma once

#include "VKEasyHeader.h"

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

		uint32_t currentImageIndex = 0;

		// Callback functions container
		std::vector<void(*)()> callbacks_onCreate;
		std::vector<void(*)()> callbacks_onDestroy;

	public:
		Swapchain() = default;

		/*
		 * Function used to build the swapchain 
		 */
		result_t Build(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);

		result_t ReBuild();

		void Destroy();

		result_t SwapImage(VkSemaphore semaphore_imageIsAvailable);

		result_t PresentImage(VkPresentInfoKHR& presentInfo);
		result_t PresentImage(VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE);

		void AddCallback_OnCreate(void(*func)()) { callbacks_onCreate.push_back(func); }
		void AddCallback_OnDestroy(void(*func)()) { callbacks_onDestroy.push_back(func); }

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


