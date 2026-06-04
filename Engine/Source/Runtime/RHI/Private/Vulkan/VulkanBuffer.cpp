#include "VulkanBuffer.h"

#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <cstring>
#include <string>

namespace XEngine
{
    namespace
    {
        VkBufferUsageFlags ToVulkanBufferUsage(RHIBufferUsage usage)
        {
            VkBufferUsageFlags flags = 0;
            if (HasFlag(usage, RHIBufferUsage::Vertex)) { flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::Index)) { flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::Uniform)) { flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::Storage)) { flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::TransferSrc)) { flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT; }
            if (HasFlag(usage, RHIBufferUsage::TransferDst)) { flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; }
            return flags;
        }

        VmaMemoryUsage ToVmaMemoryUsage(RHIMemoryUsage usage)
        {
            switch (usage)
            {
            case RHIMemoryUsage::CPUToGPU:
                return VMA_MEMORY_USAGE_CPU_TO_GPU;
            case RHIMemoryUsage::GPUToCPU:
                return VMA_MEMORY_USAGE_GPU_TO_CPU;
            case RHIMemoryUsage::GPUOnly:
            default:
                return VMA_MEMORY_USAGE_GPU_ONLY;
            }
        }
    }

    VulkanBuffer::VulkanBuffer(
        VmaAllocator allocator,
        const RHIBufferDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
        : m_Allocator(allocator)
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
        bufferCreateInfo.usage = ToVulkanBufferUsage(desc.Usage);
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

        if (initialData != nullptr && initialDataSize > 0)
        {
            void* mappedData = nullptr;
            result = vmaMapMemory(m_Allocator, m_Allocation, &mappedData);
            if (result == VK_SUCCESS)
            {
                std::memcpy(mappedData, initialData, initialDataSize < desc.Size ? initialDataSize : desc.Size);
                vmaUnmapMemory(m_Allocator, m_Allocation);
            }
            else
            {
                XENGINE_LOG_ERROR("Failed to map Vulkan buffer for initial data upload");
            }
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

    VkBuffer VulkanBuffer::GetHandle() const
    {
        return m_Buffer;
    }
}
