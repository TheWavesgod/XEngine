#pragma once

#include "VkBase.h"

namespace VK
{
    /**
     *    
     */
    class CommandBuffer 
    {
        friend class CommandPool;
        
        VkCommandBuffer handle = VK_NULL_HANDLE;

    public:
        CommandBuffer() = default;
        CommandBuffer(CommandBuffer&& other) noexcept { MoveHandle; }

        // Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // Const Function 
        result_t Begin(VkCommandBufferUsageFlags usageFlags, VkCommandBufferInheritanceInfo& inheritanceInfo) const
        {
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = usageFlags;
            beginInfo.pInheritanceInfo = &inheritanceInfo;

            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
            {
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            }
            return result;
        }

        result_t Begin(VkCommandBufferUsageFlags usageFlags = 0) const
        {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = usageFlags;
                
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
            {
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            }
            return result;
        }

        result_t End() const
        {
            VkResult result = vkEndCommandBuffer(handle);
            if (result)
            {
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to end a command buffer!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
    };

    class CommandPool
    {
        VkCommandPool handle = VK_NULL_HANDLE;
        
    public:
        CommandPool() = default;
        CommandPool(VkCommandPoolCreateInfo& createInfo) { Create(createInfo); }
        CommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) { Create(queueFamilyIndex, flags); }
        CommandPool(CommandPool&& other) noexcept { MoveHandle; }
        ~CommandPool() { DestroyHandleBy(vkDestroyCommandPool); }

        // Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // Const Function
        result_t AllocateBuffers(arrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const
        {
            VkCommandBufferAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = handle,
                .level = level,
                .commandBufferCount = uint32_t(buffers.Count())
            };
            
            VkResult result = vkAllocateCommandBuffers(VkBase::Base().Device(), &allocateInfo, buffers.Pointer());
            if (result)
            {
                outStream << std::format("[ commandPool ] ERROR\nFailed to allocate command buffers!\nError code: {}\n", int32_t(result));
            }
            return result;
        }

        result_t AllocateBuffers(arrayRef<CommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const
        {
            return AllocateBuffers({ &buffers[0].handle, buffers.Count() }, level);
        }

        void FreeBuffers(arrayRef<VkCommandBuffer> buffers) const
        {
            vkFreeCommandBuffers(VkBase::Base().Device(), handle, buffers.Count(), buffers.Pointer());
            memset(buffers.Pointer(), 0, buffers.Count() * sizeof(VkCommandBuffer));
        }

        void FreeBuffers(arrayRef<CommandBuffer> buffers) const
        {
            FreeBuffers({ &buffers[0].handle, buffers.Count() });
        }

        // Non-const Function
        result_t Create(VkCommandPoolCreateInfo& createInfo)
        {
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            VkResult result = vkCreateCommandPool(VkBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
            {
                outStream << std::format("[ commandPool ] ERROR\nFailed to create a command pool!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        
        result_t Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0)
        {
            VkCommandPoolCreateInfo createInfo = {
                .flags = flags,
                .queueFamilyIndex = queueFamilyIndex
            };
            return Create(createInfo);
        }
    };
}