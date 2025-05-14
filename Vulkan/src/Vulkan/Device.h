#pragma once

#include "VKEasyHeader.h"

namespace VK
{
	class Instance;

	class PhysicalDevice
	{
		VkPhysicalDevice handle = VK_NULL_HANDLE;

		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceMemoryProperties memoryProperties;

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
		std::vector<VkPresentModeKHR> surfacePresentModes;

		std::vector<VkPhysicalDevice> availablePhysicalDevices;

		uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;

	public:
		PhysicalDevice() = default;

		result_t Create(bool enableGraphicsQueue, bool enableComputeQueue);

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		const VkPhysicalDeviceProperties& Properties() const { return properties; }
		const VkPhysicalDeviceMemoryProperties& MemoryProperties() const { return memoryProperties; }

		const VkSurfaceCapabilitiesKHR& SurfaceCapabilities() const { return surfaceCapabilities; }
		const std::vector <VkSurfaceFormatKHR>& AvailableSurfaceFormats() const { return availableSurfaceFormats; }
		const std::vector<VkPresentModeKHR>& SurfacePresentModes() const { return surfacePresentModes; }

		inline uint32_t QueueFamilyIndex_Graphics() const { return queueFamilyIndex_graphics; }
		inline uint32_t QueueFamilyIndex_Presentation() const { return queueFamilyIndex_presentation; }
		inline uint32_t QueueFamilyIndex_Compute() const { return queueFamilyIndex_compute; }

	private:
		result_t AquireAvailablePhysicalDevices();

		/*
		 * Called to specify device and check if it has all the queue index in need and get the index  
		 */
		result_t DeterminePhysicalDevice(uint32_t deviceIndex, bool enableGraphicsQueue, bool enableComputeQueue);

		/*
		 *	Called by DeterminePhysicalDevice(...), check if this physical device have the queue family index needed, and return the index to queueFamilyIndices when sccess
		 */ 
		result_t GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]);

		result_t GetSurfaceFormats();
		result_t GetSurfacePresentModes();
	};

	class LogicalDevice
	{
		VkDevice handle = VK_NULL_HANDLE;

		VkQueue queue_graphics;
		VkQueue queue_presentation;
		VkQueue queue_compute;

		std::vector<const char*> deviceExtensions;

		std::vector<void(*)()> callbacks_onCreate;
		std::vector<void(*)()> callbacks_onDestroy;

	public:
		LogicalDevice() = default;

		result_t Create(VkDeviceCreateFlags flags = 0);

		void Destroy();

		// Use before create logical device
		void AddExtension(const char* extensionName) { AddLayerOrExtension(deviceExtensions, extensionName); }

		void AddCallback_OnCreate(void(*func)()) { callbacks_onCreate.push_back(func); }
		void AddCallback_OnDestroy(void(*func)()) { callbacks_onDestroy.push_back(func); }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		VkQueue Queue_Graphics() const { return queue_graphics; }
		VkQueue Queue_Presentation() const { return queue_presentation; }
		VkQueue Queue_Compute() const { return queue_compute; }
	};
}

