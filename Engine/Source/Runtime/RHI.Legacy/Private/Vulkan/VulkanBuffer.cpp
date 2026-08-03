#include "VulkanBuffer.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <cstring>
#include <string>

namespace XEngine
{
    VulkanBuffer::VulkanBuffer(
        VulkanDevice& device,
        VmaAllocator allocator,
        const RHIBufferDesc& desc)
        : RHIBuffer(device)
        , m_Device(device.GetHandle())
        , m_Allocator(allocator)
        , m_Size(desc.Size)
    {
        if (m_Allocator == VK_NULL_HANDLE || desc.Size == 0)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan buffer with invalid allocator or size");
            return;
        }

        VkBufferCreateInfo bufferCreateInfo {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = desc.Size;
        bufferCreateInfo.usage = ToVulkanBufferUsageFlags(desc.Usage);
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo {};
        allocationCreateInfo.usage = ToVmaMemoryUsage(desc.MemoryUsage);
        if (desc.MemoryUsage == RHIMemoryUsage::CPUToGPU || desc.MemoryUsage == RHIMemoryUsage::GPUToCPU)
        {
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        VkResult result = vmaCreateBuffer(
            m_Allocator,
            &bufferCreateInfo,
            &allocationCreateInfo,
            &m_Buffer,
            &m_Allocation,
            &m_AllocationInfo);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan buffer: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return;
        }

    }

    VulkanBuffer::~VulkanBuffer()
    {
        if (m_Allocator != VK_NULL_HANDLE && m_Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
            m_Buffer = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
        }
    }

    bool VulkanBuffer::IsValid() const
    {
        return m_Buffer != VK_NULL_HANDLE;
    }

    std::size_t VulkanBuffer::GetSize() const
    {
        return m_Size;
    }

    bool VulkanBuffer::Update(const void* data, std::size_t size, std::size_t offset)
    {
        if (data == nullptr || size == 0)
        {
            return true;
        }

        if (m_Allocator == VK_NULL_HANDLE || m_Allocation == VK_NULL_HANDLE || offset + size > m_Size)
        {
            XENGINE_LOG_ERROR("Cannot update Vulkan buffer with invalid range");
            return false;
        }

        void* mappedData = nullptr;
        const VkResult result = vmaMapMemory(m_Allocator, m_Allocation, &mappedData);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to map Vulkan buffer for update: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        std::memcpy(static_cast<u8*>(mappedData) + offset, data, size);
        vmaFlushAllocation(m_Allocator, m_Allocation, offset, size);
        vmaUnmapMemory(m_Allocator, m_Allocation);
        return true;
    }

    VkBuffer VulkanBuffer::GetHandle() const
    {
        return m_Buffer;
    }
}
