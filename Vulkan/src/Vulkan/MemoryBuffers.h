#pragma once

#include "VKEasyHeader.h"
#include "Memory.h"
#include "Image.h"

namespace VK
{
    class StagingBuffer
    {
        inline static class StagingBuffer_mainThread
        {
            StagingBuffer* pointer = Create();
            StagingBuffer* Create();

        public:
            StagingBuffer& Get() const { return *pointer; }
        } stagingBuffer_mainThread;

    protected:
        BufferMemory bufferMemory;
        VkDeviceSize memoryUsage = 0;
        Image aliasedImage;

    public:
        StagingBuffer() = default;
        StagingBuffer(VkDeviceSize size) { Expand(size); }

        // Getter
        operator VkBuffer() const { return bufferMemory.BufferRef(); }
        const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
        VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
        VkImage AliasedImage() const { return aliasedImage; }

        // Const Function
        void RetrieveData(void* pData_src, VkDeviceSize size) const;

        // Non-const Function
        void Expand(VkDeviceSize size);

        void Release() { bufferMemory.~BufferMemory(); }

        void* MapMemory(VkDeviceSize size);   
        void UnmapMemory();
        
        // Function used for writing data in to buffer
        void BufferData(const void* pData_src, VkDeviceSize size);
        

        // Used fro create linear layout aliasing 2d image
        [[nodiscard]]
        VkImage AliasedImage2d(VkFormat format, VkExtent2D extent);
        
        //Static Function
        static VkBuffer Buffer_MainThread() { return stagingBuffer_mainThread.Get(); }
        static void Expand_MainThread(VkDeviceSize size) { stagingBuffer_mainThread.Get().Expand(size); }
        static void Release_MainThread() { stagingBuffer_mainThread.Get().Release(); }
        static void* MapMemory_MainThread(VkDeviceSize size) { return stagingBuffer_mainThread.Get().MapMemory(size); }
        static void UnmapMemory_MainThread() { stagingBuffer_mainThread.Get().UnmapMemory(); }
        static void BufferData_MainThread(const void* pData_src, VkDeviceSize size) { stagingBuffer_mainThread.Get().BufferData(pData_src, size); }
        static void RetrieveData_MainThread(void* pData_src, VkDeviceSize size) { stagingBuffer_mainThread.Get().RetrieveData(pData_src, size); }
        [[nodiscard]]
        static VkImage AliasedImage2d_MainThread(VkFormat format, VkExtent2D extent) { return stagingBuffer_mainThread.Get().AliasedImage2d(format, extent); }
    };

    class DeviceLocalBuffer
    {
    protected:
        BufferMemory bufferMemory;
    public:
        DeviceLocalBuffer() = default;
        DeviceLocalBuffer(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) { Create(size, desiredUsages_Without_transfer_dst); }

        // Getter
        operator VkBuffer() const { return bufferMemory.BufferRef(); }
        const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
        VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }

        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst);
        void Recreate(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst);


        // Const function
        void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const void* pData_src, VkDeviceSize size_Limited_to_65536, VkDeviceSize offset = 0) const
        {
            vkCmdUpdateBuffer(commandBuffer, bufferMemory.BufferRef(), offset, size_Limited_to_65536, pData_src);
        }

        // It is applicable to updating consecutive data blocks from the beginning of the buffer, and the data size is automatically determined
        void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const auto& data_src) const
        {
            vkCmdUpdateBuffer(commandBuffer, bufferMemory.BufferRef(), 0, sizeof data_src, &data_src);
        }

        // suitable for updating consecutive data blocks
        void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const;

        // Suitable for updating discontinuous multiple blocks of data
        void TransferData(const void* pData_src, uint32_t elementCount, VkDeviceSize elementSize, 
            VkDeviceSize stride_src, VkDeviceSize stride_dst, VkDeviceSize offset = 0) const;
        
        void TransferData(const auto& data_src) const
        {
            TransferData(&data_src, sizeof data_src);
        }

    };

    class VertexBuffer : public DeviceLocalBuffer
    {
    public:
        VertexBuffer() = default;
        VertexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) : DeviceLocalBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages) {}

        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
        }

        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
        }
    };

    class IndexBuffer :public DeviceLocalBuffer {
    public:
        IndexBuffer() = default;
        IndexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) : DeviceLocalBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages) {}

        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Create(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
        }

        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
        }
    };

    class UniformBuffer : public DeviceLocalBuffer {
    public:
        UniformBuffer() = default;
        UniformBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) : DeviceLocalBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages) {}

        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
        }

        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
        }

        //Static Function
        static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize);
    };

    class StorageBuffer : public DeviceLocalBuffer {
    public:
        StorageBuffer() = default;
        StorageBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) : DeviceLocalBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages) {}

        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
        }

        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0)
        {
            DeviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
        }

        //Static Function
        static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize);
    };

}