#pragma once

#include "VKEasyHeader.h"

namespace VK
{
    /**
     * Fence is used to sync between CPU side and Queues
     * Only have two states, signaled and unsignaled
     */
    class Fence
    {
        VkFence handle = VK_NULL_HANDLE;
        
    public:
        Fence() = default;
        Fence(VkFenceCreateInfo& createInfo) { Create(createInfo); }
        Fence(VkFenceCreateFlags flags) { Create(flags); }

        Fence(Fence&& other) noexcept { MoveHandle; }
        ~Fence();

        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // const function
        result_t Wait() const;
        
        result_t Reset() const;

        // Cause certain circumstance that wait then reset immediately always happen, so define this
        result_t WaitAndReset() const;
        
        result_t Status() const;

        // Non-const Function
        result_t Create(VkFenceCreateInfo& createInfo);
        result_t Create(VkFenceCreateFlags flags = 0);
    };

    /*
     * Used to synchronize between queues
     * It has two types, binary one and time linear
     * 
     */
    class Semaphore
    {
        VkSemaphore handle = VK_NULL_HANDLE;

    public:
        Semaphore() = default;
        Semaphore(VkSemaphoreCreateInfo& createInfo) { Create(createInfo); }
        Semaphore(VkSemaphoreCreateFlags flags) { Create(flags); }
        
        Semaphore(Semaphore&& other) noexcept { MoveHandle; }
        ~Semaphore();
        
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // Non-const functions
        result_t Create();
        result_t Create(VkSemaphoreCreateInfo& createInfo);
        result_t Create(VkSemaphoreCreateFlags flags);
    };


    /**
     * 
     */
    class Event
    {
        VkEvent handle = VK_NULL_HANDLE;

    public:
        //Event() = default;

        Event(VkEventCreateInfo& createInfo) { Create(createInfo); }

        Event(VkEventCreateFlags flags = 0) { Create(flags); }

        Event(Event&& other) noexcept { MoveHandle; }
        ~Event();
        
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        
        //Const Function
        void CmdSet(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from) const 
        {
            vkCmdSetEvent(commandBuffer, handle, stage_from);
        }

        void CmdReset(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from) const 
        {
            vkCmdResetEvent(commandBuffer, handle, stage_from);
        }

        void CmdWait(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from, VkPipelineStageFlags stage_to,
            arrayRef<VkMemoryBarrier> memoryBarriers,
            arrayRef<VkBufferMemoryBarrier> bufferMemoryBarriers,
            arrayRef<VkImageMemoryBarrier> imageMemoryBarriers) const;


        result_t Set() const;
        result_t Reset() const;
        result_t Status() const;

        //Non-const Function
        result_t Create(VkEventCreateInfo& createInfo);
        result_t Create(VkEventCreateFlags flags = 0);
    };

}

