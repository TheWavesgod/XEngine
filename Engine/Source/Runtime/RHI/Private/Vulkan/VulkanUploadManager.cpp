#include "VulkanUploadManager.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

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
    }

     VulkanUploadManager::VulkanUploadManager(VulkanDevice& ownerDevice)
        : m_Device(ownerDevice)
        , m_Allocator(ownerDevice.GetVmaAllocator())
    {
    }

    VulkanUploadManager::~VulkanUploadManager()
    {
        for (StagingEntry& entry : m_StagingPool)
        {
            if (entry.buffer != VK_NULL_HANDLE && entry.allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(m_Allocator, entry.buffer, entry.allocation);
            }
        }
        m_StagingPool.clear();
    }

    void VulkanUploadManager::UploadBuffer(
        RHIBuffer& destination,
        const void* data,
        std::size_t size,
        std::size_t offset)
    {
        // CPU-mapped buffer path — forward to RHIBuffer::Update.
        destination.Update(data, size, offset);
    }

    void VulkanUploadManager::UploadTexture(
        RHITexture& destination,
        const void* data,
        std::size_t size,
        const RHITextureSubresourceRange& subresource)
    {
        XENGINE_ASSERT(data != nullptr && size > 0, "");

        auto* vkTexture = static_cast<VulkanTexture*>(&destination);
        const RHITextureDesc& desc = destination.GetDesc();

        // Stage 4 only handles the base-mip full-extent case.
        const u32 mipCount = (subresource.MipCount == 0)
            ? (desc.MipLevels - subresource.BaseMipLevel)
            : subresource.MipCount;
        const u32 layerCount = (subresource.ArrayLayerCount == 0)
            ? (desc.ArrayLayers - subresource.BaseArrayLayer)
            : subresource.ArrayLayerCount;
        XENGINE_ASSERT(subresource.BaseMipLevel == 0 && mipCount == 1, "");
        XENGINE_ASSERT(subresource.BaseArrayLayer == 0 && layerCount == desc.ArrayLayers, "");

        VmaAllocation stagingAlloc = VK_NULL_HANDLE;
        VkBuffer stagingBuffer = AcquireStagingBuffer(stagingAlloc, size);
        if (stagingBuffer == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Failed to acquire upload staging buffer");
            return;
        }

        void* mapped = nullptr;
        const VkResult mapResult = vmaMapMemory(m_Allocator, stagingAlloc, &mapped);
        if (mapResult != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to map upload staging buffer");
            ReleaseStagingBuffer(stagingBuffer, stagingAlloc);
            return;
        }
        std::memcpy(mapped, data, size);
        vmaUnmapMemory(m_Allocator, stagingAlloc);

        m_Device.ImmediateSubmit([&](VkCommandBuffer commandBuffer)
        {
            const VkImageAspectFlags aspect =
                (desc.Format == RHIFormat::D32Float)
                    ? VK_IMAGE_ASPECT_DEPTH_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT;

            VkImageSubresourceRange range {};
            range.aspectMask = aspect;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = desc.ArrayLayers;

            // Transition UNDEFINED/SHADER_READ -> TRANSFER_DST.
            VkImageMemoryBarrier toTransfer {};
            toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toTransfer.srcAccessMask = 0;
            toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toTransfer.oldLayout = *vkTexture->GetLayoutPtr();
            toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.image = vkTexture->GetImage();
            toTransfer.subresourceRange = range;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &toTransfer);

            VkBufferImageCopy copyRegion {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = aspect;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = desc.ArrayLayers;
            copyRegion.imageOffset = { 0, 0, 0 };
            copyRegion.imageExtent = { desc.Width, desc.Height, 1 };

            vkCmdCopyBufferToImage(
                commandBuffer,
                stagingBuffer,
                vkTexture->GetImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &copyRegion);

            // Transition TRANSFER_DST -> SHADER_READ_ONLY_OPTIMAL.
            VkImageMemoryBarrier toShaderRead {};
            toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShaderRead.image = vkTexture->GetImage();
            toShaderRead.subresourceRange = range;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &toShaderRead);
        });

        *vkTexture->GetLayoutPtr() = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // ImmediateSubmit waits idle — staging is safe to keep alive, but
        // for Stage 4 we release back to the pool for simplicity.
        ReleaseStagingBuffer(stagingBuffer, stagingAlloc);
    }

    void VulkanUploadManager::FlushUploads()
    {
        // Stage 4: uploads are inline-blocking. No-op.
    }

    VkBuffer VulkanUploadManager::AcquireStagingBuffer(
        VmaAllocation& outAllocation, std::size_t minSize)
    {
        for (StagingEntry& entry : m_StagingPool)
        {
            if (entry.size >= minSize)
            {
                outAllocation = entry.allocation;
                return entry.buffer;
            }
        }

        VkBufferCreateInfo bufferCreateInfo {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = minSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        const VkResult result = vmaCreateBuffer(
            m_Allocator, &bufferCreateInfo, &allocationCreateInfo,
            &buffer, &allocation, nullptr);
        if (result != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }

        StagingEntry entry;
        entry.buffer = buffer;
        entry.allocation = allocation;
        entry.size = minSize;
        m_StagingPool.push_back(entry);

        outAllocation = allocation;
        return buffer;
    }

    void VulkanUploadManager::ReleaseStagingBuffer(VkBuffer, VmaAllocation)
    {
        // Stage 4 keeps entries in the pool. Real release happens in
        // destructor. Replace with ring buffer / fence tracking in future
        // stages.
    }
}