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

		/**
		 * Device
		 **/
	private:
		PhysicalDevice physicalDevice;
	
		LogicalDevice device;

	public:
		//Getter
		PhysicalDevice& PhysicalDevice() { return physicalDevice; }
		LogicalDevice& Device() { return device; }

		/*VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const { return availablePhysicalDevices[index]; }
		uint32_t AvailablePhysicalDeviceCount() const { return uint32_t(availablePhysicalDevices.size()); }*/
		
		result_t GetPhysicalDevice(bool enableGraphicsQueue, bool enableComputeQueue);
		result_t CreateDevice();

		//以下函数用于创建逻辑设备失败后
		VkResult CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;

		/**
		 * Swap Chain
		 **/
	private:
		Swapchain swapchain;

		// Callback functions container
		std::vector<void(*)()> callbacks_createSwapchain;
		std::vector<void(*)()> callbacks_destroySwapchain;
		
		std::vector<void(*)()> callbacks_createDevice;
		std::vector<void(*)()> callbacks_destroyDevice;

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

		void AddCallback_CreateSwapchain(void(*function)()) { callbacks_createSwapchain.push_back(function); }
		void AddCallback_DestroySwapchain(void(*function)()) { callbacks_destroySwapchain.push_back(function); }
		void AddCallback_CreateDevice(void(*function)()) { callbacks_createDevice.push_back(function); }
		void AddCallback_DestroyDevice(void(*function)()) { callbacks_destroyDevice.push_back(function); }

	private:
		uint32_t apiVersion = VK_API_VERSION_1_0;

	public:
		// Getter
		uint32_t ApiVersion() const { return apiVersion; }

		VkResult UseLatestApiVersion();


		// 该函数用于将命令缓冲区提交到用于图形的队列
		result_t SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;

		// 该函数用于在渲染循环中将命令缓冲区提交到图形队列的常见情形
		result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer,
			VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
			VkPipelineStageFlags waitDstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) const;

		// 该函数用于将命令缓冲区提交到用于图形的队列，且只使用栅栏的常见情形
		result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const;
		
		//该函数用于将命令缓冲区提交到用于计算的队列
		result_t SubmitCommandBuffer_Compute(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;

		//该函数用于将命令缓冲区提交到用于计算的队列，且只使用栅栏的常见情形
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


