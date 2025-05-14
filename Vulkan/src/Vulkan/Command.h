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
        result_t Begin(VkCommandBufferUsageFlags usageFlags, VkCommandBufferInheritanceInfo& inheritanceInfo) const;

        result_t Begin(VkCommandBufferUsageFlags usageFlags = 0) const;
        
        result_t End() const;
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
        result_t AllocateBuffers(arrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;
        result_t AllocateBuffers(arrayRef<CommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

        void FreeBuffers(arrayRef<VkCommandBuffer> buffers) const;
        void FreeBuffers(arrayRef<CommandBuffer> buffers) const;

        // Non-const Function
        result_t Create(VkCommandPoolCreateInfo& createInfo);
        result_t Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    };
}