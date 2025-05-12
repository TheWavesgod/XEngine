#pragma once

#include "Instance.h"
#include "Surface.h"

namespace VK
{
	constexpr VkExtent2D defaultWindowSize = { 1280, 720 };
	
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

		void Terminate() {
			this->~VkBase();
			physicalDevice = VK_NULL_HANDLE;
			device = VK_NULL_HANDLE;
			surface = VK_NULL_HANDLE;
			swapchain = VK_NULL_HANDLE;
			swapchainImages.resize(0);
			swapchainImageViews.resize(0);
			swapchainCreateInfo = {};
		}


	/** Instance **/
	private:
		Instance instance;

	public:
		Instance& Instance() { return instance; }

	/** Surface **/
	private:
		VkSurfaceKHR surface;
		
	public:
		VkSurfaceKHR Surface() const { return surface; }
		
		void Surface(VkSurfaceKHR newSurface) {																		
			if (!this->surface)
				this->surface = newSurface;
		}

		/**
		 * physicalDevice
		 **/
	private:
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		std::vector<VkPhysicalDevice> availablePhysicalDevices;

		VkDevice device;
		//有效的索引从0开始，因此使用特殊值VK_QUEUE_FAMILY_IGNORED（为UINT32_MAX）为队列族索引的默认值
		uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
		VkQueue queue_graphics;
		VkQueue queue_presentation;
		VkQueue queue_compute;

		std::vector<const char*> deviceExtensions;

		//该函数被DeterminePhysicalDevice(...)调用，用于检查物理设备是否满足所需的队列族类型，并将对应的队列族索引返回到queueFamilyIndices，执行成功时直接将索引写入相应成员变量
		VkResult GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t (&queueFamilyIndices)[3]);

	public:
		//Getter
		VkPhysicalDevice PhysicalDevice() const { return physicalDevice; }

		const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const { return physicalDeviceProperties; }
		const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const { return physicalDeviceMemoryProperties; }
		
		VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const { return availablePhysicalDevices[index]; }
		uint32_t AvailablePhysicalDeviceCount() const { return uint32_t(availablePhysicalDevices.size()); }

		VkDevice Device() const { return device; }
		
		uint32_t QueueFamilyIndex_Graphics() const {
		    return queueFamilyIndex_graphics;
		}
		uint32_t QueueFamilyIndex_Presentation() const {
		    return queueFamilyIndex_presentation;
		}
		uint32_t QueueFamilyIndex_Compute() const {
		    return queueFamilyIndex_compute;
		}
		VkQueue Queue_Graphics() const {
		    return queue_graphics;
		}
		VkQueue Queue_Presentation() const {
		    return queue_presentation;
		}
		VkQueue Queue_Compute() const {
		    return queue_compute;
		}

		const std::vector<const char*>& DeviceExtensions() const {
		    return deviceExtensions;
		}

		//该函数用于创建逻辑设备前
		void AddDeviceExtension(const char* extensionName) {
		    AddLayerOrExtension(deviceExtensions, extensionName);
		}
		
		//该函数用于获取物理设备
		VkResult GetPhysicalDevices();
		
		//该函数用于指定所用物理设备并调用GetQueueFamilyIndices(...)取得队列族索引
		VkResult DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true);
		
		//该函数用于创建逻辑设备，并取得队列
		VkResult CreateDevice(VkDeviceCreateFlags flags = 0);
		
		//以下函数用于创建逻辑设备失败后
		VkResult CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;
		
		void DeviceExtensions(const std::vector<const char*>& extensionNames) {
		    deviceExtensions = extensionNames;
		}

		/**
		 * Swap Chain
		 **/
	private:
		std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;

		VkSwapchainKHR swapchain;
		std::vector <VkImage> swapchainImages;
		std::vector <VkImageView> swapchainImageViews;
		// In order to rebuild the swapchain conveniently, save the createInfo of swapchain 
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

		//该函数被CreateSwapchain(...)和RecreateSwapchain()调用
		VkResult CreateSwapchain_Internal();

		// Callback functions container
		std::vector<void(*)()> callbacks_createSwapchain;
		std::vector<void(*)()> callbacks_destroySwapchain;
		
		std::vector<void(*)()> callbacks_createDevice;
		std::vector<void(*)()> callbacks_destroyDevice;

	public:
		//Getter
		const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
			return availableSurfaceFormats[index].format;
		}
		const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
			return availableSurfaceFormats[index].colorSpace;
		}
		uint32_t AvailableSurfaceFormatCount() const {
			return uint32_t(availableSurfaceFormats.size());
		}

		VkSwapchainKHR Swapchain() const {
			return swapchain;
		}
		VkImage SwapchainImage(uint32_t index) const {
			return swapchainImages[index];
		}
		VkImageView SwapchainImageView(uint32_t index) const {
			return swapchainImageViews[index];
		}
		uint32_t SwapchainImageCount() const {
			return uint32_t(swapchainImages.size());
		}
		const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const {
			return swapchainCreateInfo;
		}

		VkResult GetSurfaceFormats();

		VkResult SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat);
		
		//该函数用于创建交换链
		VkResult CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);
			
		//该函数用于重建交换链
		VkResult RecreateSwapchain();

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

	private:
		uint32_t currentImageIndex = 0;
		
	public:
		//Getter
		uint32_t CurrentImageIndex() const { return currentImageIndex; }

		result_t SwapImage(VkSemaphore semaphore_imageIsAvailable);

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

		result_t PresentImage(VkPresentInfoKHR& presentInfo);

		result_t PresentImage(VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE);

		// VKBase+
	private:
		class VkBasePlus* pPlus = nullptr;

	public:
		static VkBasePlus& Plus() { return *VKSingleton.pPlus; }
		static void Plus(VkBasePlus& plus) { if (!VKSingleton.pPlus) VKSingleton.pPlus = &plus; }
	};

	inline VkBase VkBase::VKSingleton;
}


