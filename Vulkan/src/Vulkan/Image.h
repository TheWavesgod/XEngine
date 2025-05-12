#pragma once

#include "VkBase.h"
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
		~Image() { DestroyHandleBy(vkDestroyImage); }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		// Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const
		{
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};

			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(VkBase::Base().Device(), handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;

			auto GetMemoryTypeIndex = [](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties)->size_t
				{
					auto& physicalDeviceMemoryProperties = VkBase::Base().PhysicalDeviceMemoryProperties();
					for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
						if (memoryTypeBits & 1 << i &&
							(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
							return i;
					return UINT32_MAX;
				};
			memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);

			if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX &&
				desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			{
				memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			}
			//不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
			//if (memoryAllocateInfo.memoryTypeIndex == -1)
			//    outStream << std::format("[ image ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
			return memoryAllocateInfo;
		}

		result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const
		{
			VkResult result = vkBindImageMemory(VkBase::Base().Device(), handle, deviceMemory, memoryOffset);
			if (result)
			{
				outStream << std::format("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			}
			return result;
		}

		// Non-const Function
		result_t Create(VkImageCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			VkResult result = vkCreateImage(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
	};

	class ImageMemory :Image, DeviceMemory
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
		//以下三个函数仅用于Create(...)可能执行失败的情况
		result_t CreateImage(VkImageCreateInfo& createInfo)
		{
			return Image::Create(createInfo);
		}

		result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties)
		{
			VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
			if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
			{
				return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
			}
			return Allocate(allocateInfo);
		}

		result_t BindMemory()
		{
			if (VkResult result = Image::BindMemory(MemoryRef()))
			{
				return result;
			}
			areBound = true;
			return VK_SUCCESS;
		}

		//分配设备内存、创建图像、绑定
		result_t Create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties)
		{
			VkResult result;
			false || //这行用来应对Visual Studio中代码的对齐
				(result = CreateImage(createInfo)) || //用||短路执行
				(result = AllocateMemory(desiredMemoryProperties)) ||
				(result = BindMemory());
			return result;
		}
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
		~ImageView() { DestroyHandleBy(vkDestroyImageView); }

		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Non-const Function
		result_t Create(VkImageViewCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			VkResult result = vkCreateImageView(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
			}
			return result;
		}

		result_t Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0)
		{
			VkImageViewCreateInfo createInfo = {
				.flags = flags,
				.image = image,
				.viewType = viewType,
				.format = format,
				.subresourceRange = subresourceRange
			};
			return Create(createInfo);
		}
	};
}