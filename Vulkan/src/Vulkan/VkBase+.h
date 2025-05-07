#pragma once

#include "VkBase.h"
#include "VKFormat.h"

namespace VK
{
    struct GraphicsPipelineCreateInfoPack
    {
        VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        // Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

        std::vector<VkVertexInputBindingDescription> vertexInputBindings;
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;

        // Input Assembly 
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

        // Tessellation
        VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

        // Viewport
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

        std::vector<VkViewport> viewports;
        std::vector<VkRect2D> scissors;
        uint32_t dynamicViewportCount = 1; // 动态视口/剪裁不会用到上述的vector，因此动态视口和剪裁的个数向这俩变量手动指定
        uint32_t dynamicScissorCount = 1;

        // Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

        // Multisample
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

        // Depth & Stencil
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

        // Color Blend
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;

        // Dynamic
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        
        std::vector<VkDynamicState> dynamicStates;

        // -------------------------------------------------------------------------
        GraphicsPipelineCreateInfoPack()
        {
            SetCreateInfos();
            //若非派生管线，createInfo.basePipelineIndex不得为0，设置为-1
            createInfo.basePipelineIndex = -1;
        }

        GraphicsPipelineCreateInfoPack(const GraphicsPipelineCreateInfoPack& other) noexcept {
            createInfo = other.createInfo;
            SetCreateInfos();

            vertexInputStateCreateInfo = other.vertexInputStateCreateInfo;
            inputAssemblyStateCreateInfo = other.inputAssemblyStateCreateInfo;
            tessellationStateCreateInfo = other.tessellationStateCreateInfo;
            viewportStateCreateInfo = other.viewportStateCreateInfo;
            rasterizationStateCreateInfo = other.rasterizationStateCreateInfo;
            multisampleStateCreateInfo = other.multisampleStateCreateInfo;
            depthStencilStateCreateInfo = other.depthStencilStateCreateInfo;
            colorBlendStateCreateInfo = other.colorBlendStateCreateInfo;
            dynamicStateCreateInfo = other.dynamicStateCreateInfo;

            shaderStages = other.shaderStages;
            vertexInputBindings = other.vertexInputBindings;
            vertexInputAttributes = other.vertexInputAttributes;
            viewports = other.viewports;
            scissors = other.scissors;
            colorBlendAttachmentStates = other.colorBlendAttachmentStates;
            dynamicStates = other.dynamicStates;
            UpdateAllArrayAddresses();
        }

        //Getter，
        operator VkGraphicsPipelineCreateInfo& () { return createInfo; }

        //Non-const Function
        void UpdateAllArrays()
        {
            createInfo.stageCount = shaderStages.size();
            vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputBindings.size();
            vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputAttributes.size();
            viewportStateCreateInfo.viewportCount = viewports.size() ? uint32_t(viewports.size()) : dynamicViewportCount;
            viewportStateCreateInfo.scissorCount = scissors.size() ? uint32_t(scissors.size()) : dynamicScissorCount;
            colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
            dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
            UpdateAllArrayAddresses();
        }

    private:
        void SetCreateInfos()
        {
            createInfo.pVertexInputState = &vertexInputStateCreateInfo;
            createInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
            createInfo.pTessellationState = &tessellationStateCreateInfo;
            createInfo.pViewportState = &viewportStateCreateInfo;
            createInfo.pRasterizationState = &rasterizationStateCreateInfo;
            createInfo.pMultisampleState = &multisampleStateCreateInfo;
            createInfo.pDepthStencilState = &depthStencilStateCreateInfo;
            createInfo.pColorBlendState = &colorBlendStateCreateInfo;
            createInfo.pDynamicState = &dynamicStateCreateInfo;
        }

        void UpdateAllArrayAddresses()
        {
            createInfo.pStages = shaderStages.data();
            vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindings.data();
            vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();
            viewportStateCreateInfo.pViewports = viewports.data();
            viewportStateCreateInfo.pScissors = scissors.data();
            colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();
            dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
        }
    };

    class VkBasePlus
    {
        VkFormatProperties formatProperties[std::size(formatInfos_v1_0)] = {};
        CommandPool commandPool_graphics;
        CommandPool commandPool_presentation;
        CommandPool commandPool_compute;
        CommandBuffer commandBuffer_transfer;//从commandPool_graphics分配
        CommandBuffer commandBuffer_presentation;
        
        static VkBasePlus singleton;

        VkBasePlus() {
            //在创建逻辑设备时执行Initialize()
            auto Initialize = []()
            {
                if (VkBase::Base().QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
                {
                    singleton.commandPool_graphics.Create(VkBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                    singleton.commandPool_graphics.AllocateBuffers(singleton.commandBuffer_transfer);
                }
                    
                if (VkBase::Base().QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
                {
                    singleton.commandPool_compute.Create(VkBase::Base().QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                }
                
                if (VkBase::Base().QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
                    VkBase::Base().QueueFamilyIndex_Presentation() != VkBase::Base().QueueFamilyIndex_Graphics() &&
                    VkBase::Base().SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
                {
                    singleton.commandPool_presentation.Create(VkBase::Base().QueueFamilyIndex_Presentation(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
                    singleton.commandPool_presentation.AllocateBuffers(singleton.commandBuffer_presentation);
                }

                for (size_t i = 0; i < std::size(singleton.formatProperties); i++)
                {
                    vkGetPhysicalDeviceFormatProperties(VkBase::Base().PhysicalDevice(), VkFormat(i), &singleton.formatProperties[i]);
                }
                    
                /*待后续填充*/
            };
            //在销毁逻辑设备时执行CleanUp()
            //如果你不需要更换物理设备或在运行中重启Vulkan（皆涉及重建逻辑设备），那么此CleanUp回调非必要
            //程序运行结束时，无论是否有这个回调，graphicsBasePlus中的对象必会在析构graphicsBase前被析构掉
            auto CleanUp = []()
            {
                singleton.commandPool_graphics.~CommandPool();
                singleton.commandPool_presentation.~CommandPool();
                singleton.commandPool_compute.~CommandPool();
            };
            
            VkBase::Plus(singleton);
            
            VkBase::Base().AddCallback_CreateDevice(Initialize);
            VkBase::Base().AddCallback_DestroyDevice(CleanUp);
        }

        VkBasePlus(VkBasePlus&&) = delete;
        ~VkBasePlus() = default;

    public:
        // Getter
        const VkFormatProperties& FormatProperties(VkFormat format) const
        {
#ifndef NDEBUG
            if (uint32_t(format) >= std::size(formatInfos_v1_0))
            {
                outStream << std::format("[ FormatProperties ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n");
                abort();
            }
#endif
            return formatProperties[format];
        }
        const CommandPool& CommandPool_Graphics() const { return commandPool_graphics; }
        const CommandPool& CommandPool_Compute() const { return commandPool_compute; }
        const CommandBuffer& CommandBuffer_Transfer() const { return commandBuffer_transfer; }
        
        //Const Function
        //简化命令提交
        result_t ExecuteCommandBuffer_Graphics(VkCommandBuffer commandBuffer) const
        {
            Fence fence;
            VkSubmitInfo submitInfo = {
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer
            };
            
            VkResult result = VkBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
            if (!result) fence.Wait();
            return result;
        }
        
        //该函数专用于向呈现队列提交用于接收交换链图像的队列族所有权的命令缓冲区
        /*result_t AcquireImageOwnership_Presentation(VkSemaphore semaphore_renderingIsOver, VkSemaphore semaphore_ownershipIsTransfered, VkFence fence = VK_NULL_HANDLE) const
        {
            if (VkResult result = commandBuffer_presentation.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) return result;
            
            VkBase::Base().CmdTransferImageOwnership(commandBuffer_presentation);
            
            if (VkResult result = commandBuffer_presentation.End()) return result;
            
            return VkBase::Base().SubmitCommandBuffer_Presentation(commandBuffer_presentation, semaphore_renderingIsOver, semaphore_ownershipIsTransfered, fence);
        }*/
    };

    inline VkBasePlus VkBasePlus::singleton;

    constexpr formatInfo FormatInfo(VkFormat format)
    {
#ifndef NDEBUG
        if (uint32_t(format) >= std::size(formatInfos_v1_0))
        {
            outStream << std::format("[ FormatInfo ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n");
            abort();
        }
#endif
        return formatInfos_v1_0[uint32_t(format)];
    }
    
    constexpr VkFormat Corresponding16BitFloatFormat(VkFormat format_32BitFloat)
    {
        switch (format_32BitFloat) {
        case VK_FORMAT_R32_SFLOAT:
            return VK_FORMAT_R16_SFLOAT;
        case VK_FORMAT_R32G32_SFLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case VK_FORMAT_R32G32B32_SFLOAT:
            return VK_FORMAT_R16G16B16_SFLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        }
        return format_32BitFloat;
    }
    
    inline const VkFormatProperties& FormatProperties(VkFormat format)
    {
        return VkBase::Plus().FormatProperties(format);
    }

    class StagingBuffer
    {
        inline static class
        {
            StagingBuffer* pointer = Create();
            StagingBuffer* Create() {
                static StagingBuffer stagingBuffer;
                VkBase::Base().AddCallback_DestroyDevice([] { stagingBuffer.~StagingBuffer(); });
                return &stagingBuffer;
            }
        public:
            StagingBuffer& Get() const { return *pointer; }
        } stagingBuffer_mainThread;
        
    protected:
        BufferMemory bufferMemory;
        VkDeviceSize memoryUsage = 0;//每次映射的内存大小
        Image aliasedImage;
        
    public:
        StagingBuffer() = default;
        StagingBuffer(VkDeviceSize size) { Expand(size); }
        
        //Getter
        operator VkBuffer() const { return bufferMemory.BufferRef(); }
        const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
        VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
        VkImage AliasedImage() const { return aliasedImage; }
        
        //Const Function
        //该函数用于从缓冲区取回数据
        void RetrieveData(void* pData_src, VkDeviceSize size) const
        {
            bufferMemory.RetrieveData(pData_src, size);
        }
        
        //Non-const Function
        //该函数用于在所分配设备内存大小不够时重新分配内存
        void Expand(VkDeviceSize size)
        {
            if (size <= AllocationSize()) return;
            Release();
            VkBufferCreateInfo bufferCreateInfo = {
                .size = size,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            };
            bufferMemory.Create(bufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        }
        
        //该函数用于手动释放所有内存并销毁设备内存和缓冲区的handle
        void Release()
        {
            bufferMemory.~BufferMemory();
        }
        
        void* MapMemory(VkDeviceSize size)
        {
            Expand(size);
            void* pData_dst = nullptr;
            bufferMemory.MapMemory(pData_dst, size);
            memoryUsage = size;
            return pData_dst;
        }
        
        void UnmapMemory()
        {
            bufferMemory.UnmapMemory(memoryUsage);
            memoryUsage = 0;
        }
        
        //该函数用于向缓冲区写入数据
        void BufferData(const void* pData_src, VkDeviceSize size)
        {
            Expand(size);
            bufferMemory.BufferData(pData_src, size);
        }
        
        //该函数创建线性布局的混叠2d图像
        [[nodiscard]]
        VkImage AliasedImage2d(VkFormat format, VkExtent2D extent)
        {
            if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) return VK_NULL_HANDLE;

            VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format).sizePerPixel) * extent.width * extent.height;
            if (imageDataSize > AllocationSize()) return VK_NULL_HANDLE;

            VkImageFormatProperties imageFormatProperties = {};
            vkGetPhysicalDeviceImageFormatProperties(VkBase::Base().PhysicalDevice(),
                format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imageFormatProperties);

            //检查各个参数是否在容许范围内
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
        void Create(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst)
        {
            VkBufferCreateInfo bufferCreateInfo = {
                .size = size,
                .usage = desiredUsages_Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            };
            
            //短路执行，第一行的false||是为了对齐
            false ||
                bufferMemory.CreateBuffer(bufferCreateInfo) ||
                bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && //&&运算符优先级高于||
                bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
                bufferMemory.BindMemory();
        }
        
        void Recreate(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst)
        {
            VkBase::Base().WaitIdle(); //deviceLocalBuffer封装的缓冲区可能会在每一帧中被频繁使用，重建它之前应确保物理设备没有在使用它
            bufferMemory.~BufferMemory();
            Create(size, desiredUsages_Without_transfer_dst);
        }

        // Const function
        void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const void* pData_src, VkDeviceSize size_Limited_to_65536, VkDeviceSize offset = 0) const
        {
            vkCmdUpdateBuffer(commandBuffer, bufferMemory.BufferRef(), offset, size_Limited_to_65536, pData_src);
        }

        //适用于从缓冲区开头更新连续的数据块，数据大小自动判断
        void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const auto& data_src) const
        {
            vkCmdUpdateBuffer(commandBuffer, bufferMemory.BufferRef(), 0, sizeof data_src, &data_src);
        }

        //适用于更新连续的数据块
        void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const
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
        
        //适用于更新不连续的多块数据，stride是每组数据间的步长，这里offset当然是目标缓冲区中的offset
        void TransferData(const void* pData_src, uint32_t elementCount, VkDeviceSize elementSize, VkDeviceSize stride_src, VkDeviceSize stride_dst, VkDeviceSize offset = 0) const
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
        
        //适用于从缓冲区开头更新连续的数据块，数据大小自动判断
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
        static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize)
        {
            const VkDeviceSize& alignment = VkBase::Base().PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
            return dataSize + alignment - 1 & ~(alignment - 1);
        }
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
        static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize)
        {
            const VkDeviceSize& alignment = VkBase::Base().PhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
            return dataSize + alignment - 1 & ~(alignment - 1);
        }
    };
}
