#include "Instance.h"

namespace VK
{
	VkResult Instance::Create(VkInstanceCreateFlags flags)
	{
		if constexpr (ENABLE_DEBUG_MESSENGER)
		{
			AddLayer("VK_LAYER_KHRONOS_validation");
			AddExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.apiVersion = apiVersion;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.flags = flags;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledLayerCount = uint32_t(instanceLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
		instanceCreateInfo.enabledExtensionCount = uint32_t(instanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &handle)) 
		{
			std::cout << std::format("[ VkBase ] ERROR\nFailed to create a vulkan instance!\nError code: {}\n", int32_t(result));
			return result;
		}

		// After successfully create VkInstance, output the apiVersion of vulkan
		std::cout << std::format("Vulkan API Version: {}.{}.{}\n", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));
#ifndef NDEBUG
		// Create debug messenger just after creating vulkan instance
		CreateDebugMessenger();
#endif
		return VK_SUCCESS;
	}

	void Instance::Destroy()
	{
		if (debugMessenger)
		{
			PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessenger =
				reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(handle, "vkDestroyDebugUtilsMessengerEXT"));

			if (DestroyDebugUtilsMessenger) DestroyDebugUtilsMessenger(handle, debugMessenger, nullptr);
		}

		vkDestroyInstance(handle, nullptr);
		handle = VK_NULL_HANDLE;
	}

	VkResult Instance::CheckLayers(std::span<const char*> layersToCheck)
	{
		uint32_t layerCount = 0;
		std::vector<VkLayerProperties> availableLayers;
		if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr))
		{
			std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of instance layers!\n");
			return result;
		}

		if (layerCount)
		{
			availableLayers.resize(layerCount);
			if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()))
			{
				std::cout << std::format("[ VkBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: {}\n", int32_t(result));
				return result;
			}

			for (auto& i : layersToCheck)
			{
				bool found = false;
				for (const VkLayerProperties& j : availableLayers)
				{
					if (!strcmp(i, j.layerName))
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					i = nullptr;
				}
			}
		}
		else
		{
			for (auto& i : layersToCheck)
				i = nullptr;
		}

		return VK_SUCCESS;
	}

	VkResult Instance::CheckExtensions(std::span<const char*> extensionsToCheck, const char* layerName) const
	{
		uint32_t extensionCount = 0;
		std::vector<VkExtensionProperties> availableExtensions;
		if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr))
		{
			layerName ?
				std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:{}\n", layerName) :
				std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of instance extensions!\n");
			return result;
		}

		if (extensionCount)
		{
			availableExtensions.resize(extensionCount);
			if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data()))
			{
				std::cout << std::format("[ VkBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: {}\n", int32_t(result));
				return result;
			}

			for (auto& i : extensionsToCheck)
			{
				bool found = false;
				for (const VkExtensionProperties& j : availableExtensions)
				{
					if (!strcmp(i, j.extensionName))
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					i = nullptr;
				}
			}
		}
		else
		{
			for (auto& i : extensionsToCheck)
				i = nullptr;
		}

		return VK_SUCCESS;
	}

	VkResult Instance::CreateDebugMessenger()
	{
		static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback =
			[](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageTypes,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData)
			->VkBool32
			{
				std::cout << std::format("{}\n\n", pCallbackData->pMessage);
				return VK_FALSE;
			};

		VkDebugUtilsMessengerCreateInfoEXT  debugUtilsMessengerCreateInfo = {};
		debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugUtilsMessengerCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
			reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(handle, "vkCreateDebugUtilsMessengerEXT"));
		if (!vkCreateDebugUtilsMessenger)
		{
			std::cout << std::format("[ VkBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n");
			return VK_RESULT_MAX_ENUM;
		}

		VkResult result = vkCreateDebugUtilsMessenger(handle, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger);
		if (result != VK_SUCCESS)
		{
			std::cout << std::format("[ VkBase ] ERROR\nFailed to create a debug messenger!\nError code: {}\n", int32_t(result));
		}
		return result;
	}
}