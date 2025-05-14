#pragma once

#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "Swapchain.h"

namespace VK
{
	class VkBase
	{
		static VkBase VKSingleton;

		VkBase() = default;
		VkBase(VkBase&&) = delete;

		~VkBase();

	public:
		static VkBase& Base()
		{
			return VKSingleton;
		}

	/** Instance **/
	private:
		Instance instance;

	public:
		Instance& Instance() { return instance; }
		VkResult CreateInstance() { return instance.Create(apiVersion); }

	/** Surface **/
	private:
		Surface surface;
		
	public:
		Surface Surface() const { return surface; }
		VkSurfaceKHR& SurfaceRef() { return surface.Ref(); }

	/** Device **/
	private:
		PhysicalDevice physicalDevice;
		LogicalDevice device;

	public:
		//Getter
		PhysicalDevice& PhysicalDevice() { return physicalDevice; }
		LogicalDevice& Device() { return device; }

		/*VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const { return availablePhysicalDevices[index]; }
		uint32_t AvailablePhysicalDeviceCount() const { return uint32_t(availablePhysicalDevices.size()); }*/
		
		result_t SetPhysicalDevice(bool enableGraphicsQueue, bool enableComputeQueue);
		result_t CreateDevice();

		//以下函数用于创建逻辑设备失败后
		VkResult CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;

		/** Swapchain **/
	private:
		Swapchain swapchain;

	public:
		result_t BuildSwapchain(bool limitFrameRate = true);

		//Getter
		/*const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
			return availableSurfaceFormats[index].format;
		}
		const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
			return availableSurfaceFormats[index].colorSpace;
		}
		uint32_t AvailableSurfaceFormatCount() const {
			return uint32_t(availableSurfaceFormats.size());
		}*/

		Swapchain& Swapchain() { return swapchain; }

		VkImage SwapchainImage(uint32_t index) const { return swapchain.Images()[index]; }
		VkImageView SwapchainImageView(uint32_t index) { return swapchain.ImageViews()[index]; }
		uint32_t SwapchainImageCount() const { return uint32_t(swapchain.Images().size()); }
		VkSwapchainCreateInfoKHR& SwapchainCreateInfo() { return swapchain.SwapchainCreateInfo(); }

		VkResult RecreateDevice(VkDeviceCreateFlags flags = 0);

		VkResult WaitIdle() const;

		void AddCallback_CreateSwapchain(void(*function)()) { swapchain.AddCallback_OnCreate(function); }
		void AddCallback_DestroySwapchain(void(*function)()) { swapchain.AddCallback_OnDestroy(function); }
		void AddCallback_CreateDevice(void(*function)()) { device.AddCallback_OnCreate(function); }
		void AddCallback_DestroyDevice(void(*function)()) { device.AddCallback_OnDestroy(function); }

	private:
		uint32_t apiVersion = VK_API_VERSION_1_0;

	public:
		// Getter
		uint32_t ApiVersion() const { return apiVersion; }

		VkResult UseLatestApiVersion();


		// Submit recorded command to graphic queue
		result_t SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;

		// Submit recorded command to graphic queue, most common circumstance
		result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer,
			VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
			VkPipelineStageFlags waitDstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) const;

		// Submit recorded command to graphic queue, only use fence
		result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const;
		
		// Submit recorded command to compute queue
		result_t SubmitCommandBuffer_Compute(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;

		// Submit recorded command to compute queue,，only use fence
		result_t SubmitCommandBuffer_Compute(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const;

		// VKBase+
	private:
		class VkBasePlus* pPlus = nullptr;

	public:
		static VkBasePlus& Plus() { return *VKSingleton.pPlus; }
		static void Plus(VkBasePlus& plus) { if (!VKSingleton.pPlus) VKSingleton.pPlus = &plus; }
	};

	inline VkBase VkBase::VKSingleton;
}


