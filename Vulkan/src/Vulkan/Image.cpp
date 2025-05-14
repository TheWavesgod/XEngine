#include "Image.h"
#include "VkBase.h"

namespace VK
{
	Image::~Image()
	{
		DestroyHandleBy(vkDestroyImage);
	}

	VkMemoryAllocateInfo Image::MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const
	{
		VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
		};

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(VkBase::Base().Device(), handle, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;

		auto GetMemoryTypeIndex = [](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties)->size_t
			{
				auto& physicalDeviceMemoryProperties = VkBase::Base().PhysicalDevice().MemoryProperties();
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

	result_t Image::BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset) const
	{
		VkResult result = vkBindImageMemory(VkBase::Base().Device(), handle, deviceMemory, memoryOffset);
		if (result)
		{
			outStream << std::format("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Image::Create(VkImageCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		VkResult result = vkCreateImage(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t ImageMemory::AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties)
	{
		VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
		if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDevice().MemoryProperties().memoryTypeCount)
		{
			return VK_RESULT_MAX_ENUM;
		}
		return Allocate(allocateInfo);
	}

	result_t ImageMemory::BindMemory()
	{
		if (VkResult result = Image::BindMemory(MemoryRef()))
		{
			return result;
		}
		areBound = true;
		return VK_SUCCESS;
	}

	result_t ImageMemory::Create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties)
	{
		VkResult result;
		false ||
			(result = CreateImage(createInfo)) ||
			(result = AllocateMemory(desiredMemoryProperties)) ||
			(result = BindMemory());
		return result;
	}

	ImageView::~ImageView()
	{
		DestroyHandleBy(vkDestroyImageView);
	}

	result_t ImageView::Create(VkImageViewCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		VkResult result = vkCreateImageView(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t ImageView::Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags)
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
}