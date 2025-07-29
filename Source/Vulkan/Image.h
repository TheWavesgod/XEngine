#pragma once

#include "Memory.h"

namespace VK
{
	class Image
	{
		VkImage handle = VK_NULL_HANDLE;

	public:
		Image() = default;
		Image(VkImageCreateInfo& createInfo) { Create(createInfo); }

		Image(Image&& other) noexcept { MoveHandle; }
		~Image();

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		// Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const;
		
		result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const;

		// Non-const Function
		result_t Create(VkImageCreateInfo& createInfo);
	};

	class ImageMemory : Image, DeviceMemory
	{
	public:
		ImageMemory() = default;
		ImageMemory(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) { Create(createInfo, desiredMemoryProperties); }

		ImageMemory(ImageMemory&& other) noexcept : Image(std::move(other)), DeviceMemory(std::move(other))
		{
			areBound = other.areBound;
			other.areBound = false;
		}
		~ImageMemory() { areBound = false; }

		//Getter
		VkImage ImageRef() const { return static_cast<const Image&>(*this); }
		const VkImage* AddressOfImage() const { return Image::Address(); }
		VkDeviceMemory MemoryRef() const { return static_cast<const DeviceMemory&>(*this); }
		const VkDeviceMemory* AddressOfMemory() const { return DeviceMemory::Address(); }
		bool AreBound() const { return areBound; }
		using DeviceMemory::AllocationSize;
		using DeviceMemory::MemoryProperties;

		//Non-const Function
		result_t CreateImage(VkImageCreateInfo& createInfo)
		{
			return Image::Create(createInfo);
		}

		result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties);

		result_t BindMemory();

		result_t Create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties);
	};

	class ImageView
	{
		VkImageView handle = VK_NULL_HANDLE;

	public:
		ImageView() = default;
		ImageView(VkImageViewCreateInfo& createInfo) { Create(createInfo); }
		ImageView(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0)
		{
			Create(image, viewType, format, subresourceRange, flags);
		}

		ImageView(ImageView&& other) noexcept { MoveHandle; }
		~ImageView();

		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Non-const Function
		result_t Create(VkImageViewCreateInfo& createInfo);

		result_t Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0);
	};
}