#pragma once

#include "VkBase.h"

namespace VK
{
    /**
     * Fence is used to sync between CPU side and Queues
     * Only have two states, singaled and unsingaled
     */
    class Fence
    {
        VkFence handle = VK_NULL_HANDLE;
        
    public: 
        Fence(VkFenceCreateInfo& createInfo) {
            Create(createInfo);
        }

        Fence(VkFenceCreateFlags flags = 0) {
            Create(flags);
        }

        Fence(Fence&& other) noexcept { MoveHandle; }
        ~Fence() { DestroyHandleBy(vkDestroyFence); }

        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // const function
        result_t Wait() const
        {
            VkResult result = vkWaitForFences(VkBase::Base().Device(), 1, &handle, false, UINT64_MAX);
            if (result)
            {
                outStream << std::format("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        
        result_t Reset() const
        {
            VkResult result = vkResetFences(VkBase::Base().Device(), 1, &handle);
            if (result)
            {
                outStream << std::format("[ fence ] ERROR\nFailed to reset the fence!\nError code: {}\n", int32_t(result));
            }
            return result;
        }

        //因为“等待后立刻重置”的情形经常出现，定义此函数
        result_t WaitAndReset() const
        {
            VkResult result = Wait();
            result || (result = Reset());
            return result;
        }
        
        result_t Status() const {
            VkResult result = vkGetFenceStatus(VkBase::Base().Device(), handle);
            if (result < 0) //vkGetFenceStatus(...)成功时有两种结果，所以不能仅仅判断result是否非0
            {
                outStream << std::format("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
            }
            return result;
        }

        //Non-const Function
        result_t Create(VkFenceCreateInfo& createInfo) 
        {
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VkResult result = vkCreateFence(VkBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
            {
                outStream << std::format("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        
        result_t Create(VkFenceCreateFlags flags = 0)
        {
            VkFenceCreateInfo createInfo = {
                .flags = flags
            };
            return Create(createInfo);
        }
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
        Semaphore(VkSemaphoreCreateInfo& createInfo)
        {
            Create(createInfo);
        }
        
        //默认构造器创建未置位的信号量
        Semaphore(/*VkSemaphoreCreateFlags flags*/)
        {
            Create();
        }
        
        Semaphore(Semaphore&& other) noexcept { MoveHandle; }
        ~Semaphore() { DestroyHandleBy(vkDestroySemaphore); }
        
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // Non-const functions
        result_t Create(VkSemaphoreCreateInfo& createInfo)
        {
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkResult result = vkCreateSemaphore(VkBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
            {
                outStream << std::format("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
        
        result_t Create(/*VkSemaphoreCreateFlags flags*/)
        {
            VkSemaphoreCreateInfo createInfo = {};
            return Create(createInfo);
        }
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

