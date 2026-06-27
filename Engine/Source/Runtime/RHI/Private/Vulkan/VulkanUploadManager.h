#pragma once

#include <XEngine/RHI/RHIUploadManager.h>

#include <volk.h>
#include <vk_mem_alloc.h>
#include <vector>

namespace XEngine
{
    class VulkanDevice;

    class VulkanUploadManager final : public RHIUploadManager
    {
    public:
        explicit VulkanUploadManager(VulkanDevice& ownerDevice);
        ~VulkanUploadManager() override;

        void UploadBuffer(
            RHIBuffer& destination,
            const void* data,
            std::size_t size,
            std::size_t offset = 0) override;

        void UploadTexture(
            RHITexture& destination,
            const void* data,
            std::size_t size,
            const RHITextureSubresourceRange& subresource = AllSubresources()) override;

        void FlushUploads() override;

    private:
        // Acquire a staging buffer of at least `minSize` bytes.
        VkBuffer AcquireStagingBuffer(VmaAllocation& outAllocation, std::size_t minSize);
        void ReleaseStagingBuffer(VkBuffer buffer, VmaAllocation allocation);

        VulkanDevice& m_Device;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;

        struct StagingEntry
        {
            VmaAllocation allocation = VK_NULL_HANDLE;
            VkBuffer buffer = VK_NULL_HANDLE;
            std::size_t size = 0;
        };
        std::vector<StagingEntry> m_StagingPool;
    };
}