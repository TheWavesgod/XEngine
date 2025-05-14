#pragma once

#include "VKEasyHeader.h"

namespace VK
{
	class Instance
	{
		VkInstance handle = VK_NULL_HANDLE;

		std::vector<const char*> instanceLayers;
		std::vector<const char*> instanceExtensions;

		VkDebugUtilsMessengerEXT debugMessenger;

	public:
		Instance() = default;

		// Function used before create instance
		void AddLayer(const char* layerName) { AddLayerOrExtension(instanceLayers, layerName); }
		void AddExtension(const char* extensionName) { AddLayerOrExtension(instanceExtensions, extensionName); }

		VkResult Create(uint32_t apiVersion, VkInstanceCreateFlags flags = 0);
		void Destroy();

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		const std::vector<const char*>& Layers() const { return instanceLayers; }
		const std::vector<const char*>& Extensions() const { return instanceExtensions; }

		void Layers(const std::vector<const char*>& layerNames) { instanceLayers = layerNames; }
		void Extensions(const std::vector<const char*>& extensionNames) { instanceExtensions = extensionNames; }

		// Check after failed to create instance
		VkResult CheckLayers(std::span<const char*> layersToCheck);
		VkResult CheckExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;

	private:
		VkResult CreateDebugMessenger();
	};
}


