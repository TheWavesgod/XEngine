#pragma once

#include "VkBase.h"

namespace VK
{
	class DeviceMemory
	{
		VkDeviceMemory handle = VK_NULL_HANDLE;
		// Actual allocated memory size 
		VkDeviceSize allocationSize = 0;
		// Memory attribute
		VkMemoryPropertyFlags memoryProperties = 0;

		// Modify the range of Non-host coherent Memory, when try to map the memory
		VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const
		{
			const VkDeviceSize& nonCoherentAtomSize = VkBase::Base().PhysicalDeviceProperties().limits.nonCoherentAtomSize;
			VkDeviceSize _offset = offset;
			offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
			size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
			return _offset - offset;
		}

	protected:
		class
		{
			friend class BufferMemory;
			friend class ImageMemory;
			bool value = false;
			operator bool() const { return value; }
			auto& operator = (bool value) { this->value = value; return *this; }
		} areBound;

	public:
		DeviceMemory() = default;
		DeviceMemory(VkMemoryAllocateInfo& allocateInfo) { Allocate(allocateInfo); }
		DeviceMemory(DeviceMemory&& other) noexcept
		{
			MoveHandle;
			allocationSize = other.allocationSize;
			memoryProperties = other.memoryProperties;
			other.allocationSize = 0;
			other.memoryProperties = 0;
		}
		~DeviceMemory() { DestroyHandleBy(vkFreeMemory); allocationSize = 0; memoryProperties = 0; }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		VkDeviceSize AllocationSize() const { return allocationSize; }
		VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }

		// Const function
		// 映射host visible的内存区
		result_t MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const
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

		// 取消映射host visible的内存区
		result_t UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const
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

		// BufferData(...)用于方便地更新设备内存区，适用于用memcpy(...)向内存区写入数据后立刻取消映射的情况
		result_t BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const
		{
			void* pData_dst;
			if (VkResult result = MapMemory(pData_dst, size, offset))
			{
				return result;
			}
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}

		result_t BufferData(const auto& data_src) const
		{
			return BufferData(&data_src, sizeof data_src);
		}

		// RetrieveData(...)用于方便地从设备内存区取回数据，适用于用memcpy(...)从内存区取得数据后立刻取消映射的情况
		result_t RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const
		{
			void* pData_src;
			if (VkResult result = MapMemory(pData_src, size, offset))
				return result;
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}

		//Non-const Function
		result_t Allocate(VkMemoryAllocateInfo& allocateInfo)
		{
			if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
			{
				outStream << std::format("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
				return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
			}

			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			if (VkResult result = vkAllocateMemory(VkBase::Base().Device(), &allocateInfo, nullptr, &handle))
			{
				outStream << std::format("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
				return result;
			}
			//记录实际分配的内存大小
			allocationSize = allocateInfo.allocationSize;
			//取得内存属性
			memoryProperties = VkBase::Base().PhysicalDeviceMemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
			return VK_SUCCESS;
		}
	};

	class Buffer
	{
		VkBuffer handle = VK_NULL_HANDLE;

	public:
		Buffer() = default;
		Buffer(VkBufferCreateInfo& createInfo) { Create(createInfo); }
		Buffer(Buffer&& other) noexcept { MoveHandle; }
		~Buffer() { DestroyHandleBy(vkDestroyBuffer); }

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
			vkGetBufferMemoryRequirements(VkBase::Base().Device(), handle, &memoryRequirements);

			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
			auto& physicalDeviceMemoryProperties = VkBase::Base().PhysicalDeviceMemoryProperties();
			for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
			{
				if (memoryRequirements.memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
				{
					memoryAllocateInfo.memoryTypeIndex = i;
					break;
				}
			}

			//不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
			//if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
			//    outStream << std::format("[ buffer ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
			return memoryAllocateInfo;
		}

		result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const
		{
			VkResult result = vkBindBufferMemory(VkBase::Base().Device(), handle, deviceMemory, memoryOffset);
			if (result)
			{
				outStream << std::format("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			}
			return result;
		}

		// Non-const Function
		result_t Create(VkBufferCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			VkResult result = vkCreateBuffer(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
	};

	class BufferMemory : Buffer, DeviceMemory
	{
	public:
		BufferMemory() = default;
		BufferMemory(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) { Create(createInfo, desiredMemoryProperties); }
		BufferMemory(BufferMemory&& other) noexcept : Buffer(std::move(other)), DeviceMemory(std::move(other))
		{
			areBound = other.areBound;
			other.areBound = false;
		}
		~BufferMemory() { areBound = false; }

		// Getter
		// 不定义到VkBuffer和VkDeviceMemory的转换函数，因为32位下这俩类型都是uint64_t的别名，会造成冲突（虽然，谁他妈还用32位PC！）
		VkBuffer BufferRef() const { return static_cast<const Buffer&>(*this); }
		const VkBuffer* AddressOfBuffer() const { return Buffer::Address(); }
		VkDeviceMemory MemoryRef() const { return static_cast<const DeviceMemory&>(*this); }
		const VkDeviceMemory* AddressOfMemory() const { return DeviceMemory::Address(); }
		//若areBond为true，则成功分配了设备内存、创建了缓冲区，且成功绑定在一起
		bool AreBound() const { return areBound; }
		using DeviceMemory::AllocationSize;
		using DeviceMemory::MemoryProperties;

		// Const Function
		using DeviceMemory::MapMemory;
		using DeviceMemory::UnmapMemory;
		using DeviceMemory::BufferData;
		using DeviceMemory::RetrieveData;

		// Non-const Function
		// 以下三个函数仅用于Create(...)可能执行失败的情况
		result_t CreateBuffer(VkBufferCreateInfo& createInfo)
		{
			return Buffer::Create(createInfo);
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
			if (VkResult result = Buffer::BindMemory(MemoryRef()))
				return result;
			areBound = true;
			return VK_SUCCESS;
		}

		//分配设备内存、创建缓冲、绑定
		result_t Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties)
		{
			VkResult result;
			false || //这行用来应对Visual Studio中代码的对齐
				(result = CreateBuffer(createInfo)) || //用||短路执行
				(result = AllocateMemory(desiredMemoryProperties)) ||
				(result = BindMemory());
			return result;
		}
	};

	class BufferView
	{
		VkBufferView handle = VK_NULL_HANDLE;

	public:
		BufferView() = default;
		BufferView(VkBufferViewCreateInfo& createInfo) { Create(createInfo); }
		BufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*VkBufferViewCreateFlags flags*/) { Create(buffer, format, offset, range); }
		BufferView(BufferView&& other) noexcept { MoveHandle; }
		~BufferView() { DestroyHandleBy(vkDestroyBufferView); }

		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Non-const Function
		result_t Create(VkBufferViewCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			VkResult result = vkCreateBufferView(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ bufferView ] ERROR\nFailed to create a buffer view!\nError code: {}\n", int32_t(result));
			}
			return result;
		}

		result_t Create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*VkBufferViewCreateFlags flags*/)
		{
			VkBufferViewCreateInfo createInfo = {
				.buffer = buffer,
				.format = format,
				.offset = offset,
				.range = range
			};
			return Create(createInfo);
		}
	};
}


