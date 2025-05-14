#pragma once

#include "VKEasyHeader.h"

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
		VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const;

	protected:
		// Used for bufferMemory and imageMemory, define here in order to save 8 bit
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

		DeviceMemory(DeviceMemory&& other) noexcept;
		~DeviceMemory(); 

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		VkDeviceSize AllocationSize() const { return allocationSize; }
		VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }

		// Const function
		result_t MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const;

		result_t UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const;

		// BufferData(...) used for update device memory conveniently
		// Suitable for unmap right after using memcpy(...) to write data
		result_t BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const;

		result_t BufferData(const auto& data_src) const;
		
		// RetrieveData(...) used for get data back from device memory conveniently
		// Suitable for unmap right after using memcpy(...) to read data
		result_t RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const;
		
		//Non-const Function
		result_t Allocate(VkMemoryAllocateInfo& allocateInfo);
	};

	class Buffer
	{
		VkBuffer handle = VK_NULL_HANDLE;

	public:
		Buffer() = default;
		Buffer(VkBufferCreateInfo& createInfo) { Create(createInfo); }

		Buffer(Buffer&& other) noexcept { MoveHandle; }
		~Buffer();

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		// Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const;

		result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const;

		// Non-const Function
		result_t Create(VkBufferCreateInfo& createInfo);
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
		VkBuffer BufferRef() const { return static_cast<const Buffer&>(*this); }
		const VkBuffer* AddressOfBuffer() const { return Buffer::Address(); }
		VkDeviceMemory MemoryRef() const { return static_cast<const DeviceMemory&>(*this); }
		const VkDeviceMemory* AddressOfMemory() const { return DeviceMemory::Address(); }

		// if areBond is true, so device momeory allocating success, buffers createdm and bind togethre
		bool AreBound() const { return areBound; }
		using DeviceMemory::AllocationSize;
		using DeviceMemory::MemoryProperties;

		// Const Function
		using DeviceMemory::MapMemory;
		using DeviceMemory::UnmapMemory;
		using DeviceMemory::BufferData;
		using DeviceMemory::RetrieveData;

		// Non-const Function
		result_t CreateBuffer(VkBufferCreateInfo& createInfo);

		result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties);
		
		result_t BindMemory();

		// Allocate device memory, create framebuffer, binding
		result_t Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties);
		
	};

	class BufferView
	{
		VkBufferView handle = VK_NULL_HANDLE;

	public:
		BufferView() = default;
		BufferView(VkBufferViewCreateInfo& createInfo) { Create(createInfo); }
		BufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0
			/*VkBufferViewCreateFlags flags*/) { Create(buffer, format, offset, range); }
		
		BufferView(BufferView&& other) noexcept { MoveHandle; }
		~BufferView();

		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Non-const Function
		result_t Create(VkBufferViewCreateInfo& createInfo);
		result_t Create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*VkBufferViewCreateFlags flags*/);
	};
}


