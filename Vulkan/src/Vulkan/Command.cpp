#include "Command.h"

namespace VK
{
	result_t CommandBuffer::Begin(VkCommandBufferUsageFlags usageFlags, VkCommandBufferInheritanceInfo& inheritanceInfo) const
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

    result_t CommandBuffer::Begin(VkCommandBufferUsageFlags usageFlags) const
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

    result_t CommandBuffer::End() const
    {
        VkResult result = vkEndCommandBuffer(handle);
        if (result)
        {
            outStream << std::format("[ commandBuffer ] ERROR\nFailed to end a command buffer!\nError code: {}\n", int32_t(result));
        }
        return result;
    }

    result_t CommandPool::AllocateBuffers(arrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level) const
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

    result_t CommandPool::AllocateBuffers(arrayRef<CommandBuffer> buffers, VkCommandBufferLevel level) const
    {
        return AllocateBuffers({ &buffers[0].handle, buffers.Count() }, level);
    }

    result_t CommandPool::Create(VkCommandPoolCreateInfo& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        VkResult result = vkCreateCommandPool(VkBase::Base().Device(), &createInfo, nullptr, &handle);
        if (result)
        {
            outStream << std::format("[ commandPool ] ERROR\nFailed to create a command pool!\nError code: {}\n", int32_t(result));
        }
        return result;
    }

    result_t CommandPool::Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo createInfo = {
            .flags = flags,
            .queueFamilyIndex = queueFamilyIndex
        };
        return Create(createInfo);
    }
}