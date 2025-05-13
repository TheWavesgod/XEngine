#pragma once

#include "VkBase.h"

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
        Fence(VkFenceCreateInfo& createInfo) { Create(createInfo); }

        Fence(VkFenceCreateFlags flags = 0) { Create(flags); }

        Fence(Fence&& other) noexcept { MoveHandle; }
        ~Fence() { DestroyHandleBy(vkDestroyFence); }                 

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
        Semaphore(VkSemaphoreCreateInfo& createInfo) { Create(createInfo); }
        
        //默认构造器创建未置位的信号量
        Semaphore(/*VkSemaphoreCreateFlags flags*/) { Create(); }
        
        Semaphore(Semaphore&& other) noexcept { MoveHandle; }
        ~Semaphore() { DestroyHandleBy(vkDestroySemaphore); }
        
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // Non-const functions
        result_t Create(VkSemaphoreCreateInfo& createInfo);
        result_t Create(/*VkSemaphoreCreateFlags flags*/);
    };


    /**
     * 
     */
    class Event
    {
        VkEvent handle = VK_NULL_HANDLE;

    public:
        //Event() = default;

        Event(VkEventCreateInfo& createInfo) 
        {
            Create(createInfo);
        }

        Event(VkEventCreateFlags flags = 0) 
        {
            Create(flags);
        }

        Event(Event&& other) noexcept { MoveHandle; }
        ~Event() { DestroyHandleBy(vkDestroyEvent); }
        
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
            arrayRef<VkImageMemoryBarrier> imageMemoryBarriers) const 
        {
            for (auto& i : memoryBarriers)
                i.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            for (auto& i : bufferMemoryBarriers)
                i.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            for (auto& i : imageMemoryBarriers)
                i.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vkCmdWaitEvents(commandBuffer, 1, &handle, stage_from, stage_to,
                memoryBarriers.Count(), memoryBarriers.Pointer(),
                bufferMemoryBarriers.Count(), bufferMemoryBarriers.Pointer(),
                imageMemoryBarriers.Count(), imageMemoryBarriers.Pointer());
        }

        result_t Set() const {
            VkResult result = vkSetEvent(VkBase::Base().Device(), handle);
            if (result)
                outStream << std::format("[ Event ] ERROR\nFailed to singal the Event!\nError code: {}\n", int32_t(result));
            return result;
        }

        result_t Reset() const {
            VkResult result = vkResetEvent(VkBase::Base().Device(), handle);
            if (result)
                outStream << std::format("[ Event ] ERROR\nFailed to unsingal the Event!\nError code: {}\n", int32_t(result));
            return result;
        }

        result_t Status() const {
            VkResult result = vkGetEventStatus(VkBase::Base().Device(), handle);
            if (result < 0) //vkGetEventStatus(...)成功时有两种结果
                outStream << std::format("[ Event ] ERROR\nFailed to get the status of the Event!\nError code: {}\n", int32_t(result));
            return result;
        }

        //Non-const Function
        result_t Create(VkEventCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
            VkResult result = vkCreateEvent(VkBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ Event ] ERROR\nFailed to create a Event!\nError code: {}\n", int32_t(result));
            return result;
        }

        result_t Create(VkEventCreateFlags flags = 0) {
            VkEventCreateInfo createInfo = {
                .flags = flags
            };
            return Create(createInfo);
        }
    };

}

