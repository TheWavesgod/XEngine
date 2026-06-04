#pragma once

#include <XEngine/RHI/Resources/RHIBuffer.h>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace XEngine
{
    class VulkanBuffer final : public RHIBuffer
    {
    public:
        VulkanBuffer() = default;
        VulkanBuffer(VmaAllocator allocator, const RHIBufferDesc& desc, const void* initialData, std::size_t initialDataSize);
        ~VulkanBuffer() override;

        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        bool IsValid() const;
        std::size_t GetSize() const override;
        VkBuffer GetHandle() const;

    private:
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_AllocationInfo {};
        std::size_t m_Size = 0;
    };
}
