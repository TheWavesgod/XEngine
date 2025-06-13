#include "Memory.h"
#include "VkBase.h"

namespace VK
{
	VkDeviceSize DeviceMemory::AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const
	{
		const VkDeviceSize& nonCoherentAtomSize = VkBase::Base().PhysicalDevice().Properties().limits.nonCoherentAtomSize;
		VkDeviceSize _offset = offset;
		offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
		size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
		return _offset - offset;
	}

	DeviceMemory::DeviceMemory(DeviceMemory&& other) noexcept
	{
		MoveHandle;
		allocationSize = other.allocationSize;
		memoryProperties = other.memoryProperties;
		other.allocationSize = 0;
		other.memoryProperties = 0;
	}

	DeviceMemory::~DeviceMemory()
	{
		DestroyHandleBy(vkFreeMemory); 
		allocationSize = 0;
		memoryProperties = 0;
	}

	result_t DeviceMemory::MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset) const
	{
		VkDeviceSize inverseDeltaOffset;
		if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		{
			inverseDeltaOffset = AdjustNonCoherentMemoryRange(size, offset);
		}
		if (VkResult result = vkMapMemory(VkBase::Base().Device(), handle, offset, size, 0, &pData)) {
			outStream << std::format("[ deviceMemory ] ERROR\nFailed to map the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			pData = static_cast<uint8_t*>(pData) + inverseDeltaOffset;
			VkMappedMemoryRange mappedMemoryRange = {
				.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				.memory = handle,
				.offset = offset,
				.size = size
			};
			if (VkResult result = vkInvalidateMappedMemoryRanges(VkBase::Base().Device(), 1, &mappedMemoryRange)) {
				outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
				return result;
			}
		}
		return VK_SUCCESS;
	}

	result_t DeviceMemory::UnmapMemory(VkDeviceSize size, VkDeviceSize offset) const
	{
		if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		{
			AdjustNonCoherentMemoryRange(size, offset);
			VkMappedMemoryRange mappedMemoryRange = {
				.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				.memory = handle,
				.offset = offset,
				.size = size
			};
			if (VkResult result = vkFlushMappedMemoryRanges(VkBase::Base().Device(), 1, &mappedMemoryRange))
			{
				outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
				return result;
			}
		}
		vkUnmapMemory(VkBase::Base().Device(), handle);
		return VK_SUCCESS;
	}

	result_t DeviceMemory::BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset) const
	{
		void* pData_dst;
		if (VkResult result = MapMemory(pData_dst, size, offset))
		{
			return result;
		}
		memcpy(pData_dst, pData_src, size_t(size));
		return UnmapMemory(size, offset);
	}

	result_t DeviceMemory::BufferData(const auto& data_src) const
	{
		return BufferData(&data_src, sizeof data_src);
	}

	result_t DeviceMemory::RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset) const
	{
		void* pData_src;
		if (VkResult result = MapMemory(pData_src, size, offset))
			return result;
		memcpy(pData_dst, pData_src, size_t(size));
		return UnmapMemory(size, offset);
	}

	result_t DeviceMemory::Allocate(VkMemoryAllocateInfo& allocateInfo)
	{
		if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDevice().MemoryProperties().memoryTypeCount)
		{
			outStream << std::format("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
			return VK_RESULT_MAX_ENUM; 
		}

		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		if (VkResult result = vkAllocateMemory(VkBase::Base().Device(), &allocateInfo, nullptr, &handle))
		{
			outStream << std::format("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
			return result;
		}

		// Save the actual allocate memory size
		allocationSize = allocateInfo.allocationSize;

		// Get the memory properties
		memoryProperties = VkBase::Base().PhysicalDevice().MemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;

		return VK_SUCCESS;
	}

	Buffer::~Buffer()
	{
		DestroyHandleBy(vkDestroyBuffer);
	}

	VkMemoryAllocateInfo Buffer::MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const
	{
		VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
		};

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(VkBase::Base().Device(), handle, &memoryRequirements);

		memoryAllocateInfo.allocationSize = memoryRequirements.size; // TODO: Check why the actual memory here is larger than we specified 
		memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
		auto& physicalDeviceMemoryProperties = VkBase::Base().PhysicalDevice().MemoryProperties();
		for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if (memoryRequirements.memoryTypeBits & 1 << i &&
				(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
			{
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}

		/*if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
		    outStream << std::format("[ buffer ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");*/
		return memoryAllocateInfo;
	}

	result_t Buffer::BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset) const
	{
		VkResult result = vkBindBufferMemory(VkBase::Base().Device(), handle, deviceMemory, memoryOffset);
		if (result)
		{
			outStream << std::format("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Buffer::Create(VkBufferCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		VkResult result = vkCreateBuffer(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t BufferMemory::CreateBuffer(VkBufferCreateInfo& createInfo)
	{
		return Buffer::Create(createInfo);
	}

	result_t BufferMemory::AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties)
	{
		VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
		if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDevice().MemoryProperties().memoryTypeCount)
		{
			return VK_RESULT_MAX_ENUM;
		}
		return Allocate(allocateInfo);
	}

	result_t BufferMemory::BindMemory()
	{
		if (VkResult result = Buffer::BindMemory(MemoryRef()))
			return result;
		areBound = true;
		return VK_SUCCESS;
	}

	result_t BufferMemory::Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties)
	{
		VkResult result;
		false ||
			(result = CreateBuffer(createInfo)) ||
			(result = AllocateMemory(desiredMemoryProperties)) ||
			(result = BindMemory());
		return result;
	}

	BufferView::~BufferView()
	{
		DestroyHandleBy(vkDestroyBufferView);
	}

	result_t BufferView::Create(VkBufferViewCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		VkResult result = vkCreateBufferView(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ bufferView ] ERROR\nFailed to create a buffer view!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t BufferView::Create(VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize range)
	{
		VkBufferViewCreateInfo createInfo = {
			.buffer = buffer,
			.format = format,
			.offset = offset,
			.range = range
		};
		return Create(createInfo);
	}
}