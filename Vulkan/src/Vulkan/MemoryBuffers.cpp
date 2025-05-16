#include "MemoryBuffers.h"
#include "VkBase.h"
#include "VkBase+.h"

namespace VK
{
    void StagingBuffer::RetrieveData(void* pData_src, VkDeviceSize size) const
    {
        bufferMemory.RetrieveData(pData_src, size);
    }

    void StagingBuffer::Expand(VkDeviceSize size)
    {
        if (size <= AllocationSize()) return;
        Release();
        VkBufferCreateInfo bufferCreateInfo = {
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        };
        bufferMemory.Create(bufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    void* StagingBuffer::MapMemory(VkDeviceSize size)
    {
        Expand(size);
        void* pData_dst = nullptr;
        bufferMemory.MapMemory(pData_dst, size);
        memoryUsage = size;
        return pData_dst;
    }

    void StagingBuffer::UnmapMemory()
    {
        bufferMemory.UnmapMemory(memoryUsage);
        memoryUsage = 0;
    }

    void StagingBuffer::BufferData(const void* pData_src, VkDeviceSize size)
    {
        Expand(size);
        bufferMemory.BufferData(pData_src, size);
    }

    VkImage StagingBuffer::AliasedImage2d(VkFormat format, VkExtent2D extent)
    {
        if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) return VK_NULL_HANDLE;

        VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format).sizePerPixel) * extent.width * extent.height;
        if (imageDataSize > AllocationSize()) return VK_NULL_HANDLE;

        VkImageFormatProperties imageFormatProperties = {};
        vkGetPhysicalDeviceImageFormatProperties(VkBase::Base().PhysicalDevice(),
            format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imageFormatProperties);

        // Check each parameters is in allowed range
        if (extent.width > imageFormatProperties.maxExtent.width ||
            extent.height > imageFormatProperties.maxExtent.height ||
            imageDataSize > imageFormatProperties.maxResourceSize)
            return VK_NULL_HANDLE;

        VkImageCreateInfo imageCreateInfo = {
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = { extent.width, extent.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
        };
        aliasedImage.~Image();
        aliasedImage.Create(imageCreateInfo);

        VkImageSubresource subResource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
        VkSubresourceLayout subresourceLayout = {};
        vkGetImageSubresourceLayout(VkBase::Base().Device(), aliasedImage, &subResource, &subresourceLayout);
        if (subresourceLayout.size != imageDataSize) return VK_NULL_HANDLE;
        aliasedImage.BindMemory(bufferMemory.MemoryRef());
        return aliasedImage;
    }

    void DeviceLocalBuffer::Create(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst)
    {
        VkBufferCreateInfo bufferCreateInfo = {
            .size = size,
            .usage = desiredUsages_Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        };

        false ||
            bufferMemory.CreateBuffer(bufferCreateInfo) ||
            bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && // && prior than ||
            bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
            bufferMemory.BindMemory();
    }

    void DeviceLocalBuffer::Recreate(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst)
    {
        VkBase::Base().WaitIdle(); // deviceLocalBuffer may be used frequently in every frame, make sure not in using before recreate
        bufferMemory.~BufferMemory();
        Create(size, desiredUsages_Without_transfer_dst);
    }

    void DeviceLocalBuffer::TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset) const
    {
        if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            bufferMemory.BufferData(pData_src, size, offset);
            return;
        }

        StagingBuffer::BufferData_MainThread(pData_src, size);
        auto& commandBuffer = VkBase::Plus().CommandBuffer_Transfer();
        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VkBufferCopy region = { 0, offset, size };
        vkCmdCopyBuffer(commandBuffer, StagingBuffer::Buffer_MainThread(), bufferMemory.BufferRef(), 1, &region);
        commandBuffer.End();
        VkBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
    }

    void DeviceLocalBuffer::TransferData(const void* pData_src, uint32_t elementCount, 
        VkDeviceSize elementSize, VkDeviceSize stride_src, VkDeviceSize stride_dst, VkDeviceSize offset) const
    {
        if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            void* pData_dst = nullptr;
            bufferMemory.MapMemory(pData_dst, stride_dst * elementCount, offset);
            for (size_t i = 0; i < elementCount; i++)
            {
                memcpy(stride_dst * i + static_cast<uint8_t*>(pData_dst), stride_src * i + static_cast<const uint8_t*>(pData_src), size_t(elementSize));
            }
            bufferMemory.UnmapMemory(elementCount * stride_dst, offset);
            return;
        }

        StagingBuffer::BufferData_MainThread(pData_src, stride_src * elementCount);
        auto& commandBuffer = VkBase::Plus().CommandBuffer_Transfer();
        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        std::unique_ptr<VkBufferCopy[]> regions = std::make_unique<VkBufferCopy[]>(elementCount);
        for (size_t i = 0; i < elementCount; i++)
        {
            regions[i] = { stride_src * i, stride_dst * i + offset, elementSize };
        }
        vkCmdCopyBuffer(commandBuffer, StagingBuffer::Buffer_MainThread(), bufferMemory.BufferRef(), elementCount, regions.get());
        commandBuffer.End();
        VkBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
    }

    VkDeviceSize UniformBuffer::CalculateAlignedSize(VkDeviceSize dataSize)
    {
        const VkDeviceSize& alignment = VkBase::Base().PhysicalDevice().Properties().limits.minUniformBufferOffsetAlignment;
        return dataSize + alignment - 1 & ~(alignment - 1);
    }

    VkDeviceSize StorageBuffer::CalculateAlignedSize(VkDeviceSize dataSize)
    {
        const VkDeviceSize& alignment = VkBase::Base().PhysicalDevice().Properties().limits.minStorageBufferOffsetAlignment;
        return dataSize + alignment - 1 & ~(alignment - 1);
    }

    StagingBuffer* StagingBuffer::StagingBuffer_mainThread::Create()
    {
        static StagingBuffer stagingBuffer;
        VkBase::Base().AddCallback_DestroyDevice([] { stagingBuffer.~StagingBuffer(); });
        return &stagingBuffer;
    }
}