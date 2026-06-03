#include "VulkanFrameResources.h"

#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

namespace XEngine
{
    VulkanFrameResources::~VulkanFrameResources()
    {
        Destroy();
    }

    bool VulkanFrameResources::Create(VkDevice device, u32 graphicsQueueFamilyIndex, u32 swapchainImageCount)
    {
        if (m_CommandPool != VK_NULL_HANDLE)
        {
            return true;
        }

        m_Device = device;

        VkCommandPoolCreateInfo commandPoolCreateInfo {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

        VkResult result = vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPool);
        if (result != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to create Vulkan command pool");
            return false;
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = m_CommandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        result = vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, &m_CommandBuffer);
        if (result != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to allocate Vulkan command buffer");
            Destroy();
            return false;
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore);
        if (result != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to create Vulkan image available semaphore");
            Destroy();
            return false;
        }

        m_RenderFinishedSemaphores.resize(swapchainImageCount);
        for (VkSemaphore& semaphore : m_RenderFinishedSemaphores)
        {
            result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &semaphore);
            if (result != VK_SUCCESS)
            {
                XENGINE_LOG_ERROR("Failed to create Vulkan render finished semaphore");
                Destroy();
                return false;
            }
        }

        VkFenceCreateInfo fenceCreateInfo {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        result = vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_InFlightFence);
        if (result != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to create Vulkan in-flight fence");
            Destroy();
            return false;
        }

        XENGINE_LOG_INFO("Vulkan frame resources created");
        return true;
    }

    void VulkanFrameResources::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        if (m_InFlightFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_Device, m_InFlightFence, nullptr);
            m_InFlightFence = VK_NULL_HANDLE;
        }

        for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
        {
            if (semaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_Device, semaphore, nullptr);
            }
        }
        m_RenderFinishedSemaphores.clear();

        if (m_ImageAvailableSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
            m_ImageAvailableSemaphore = VK_NULL_HANDLE;
        }

        if (m_CommandBuffer != VK_NULL_HANDLE && m_CommandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_CommandBuffer);
            m_CommandBuffer = VK_NULL_HANDLE;
        }

        if (m_CommandPool != VK_NULL_HANDLE)
        {
            XENGINE_LOG_INFO("Destroying Vulkan frame resources");
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        m_Device = VK_NULL_HANDLE;
    }

    VkCommandPool VulkanFrameResources::GetCommandPool() const
    {
        return m_CommandPool;
    }

    VkCommandBuffer VulkanFrameResources::GetCommandBuffer() const
    {
        return m_CommandBuffer;
    }

    VkSemaphore VulkanFrameResources::GetImageAvailableSemaphore() const
    {
        return m_ImageAvailableSemaphore;
    }

    VkSemaphore VulkanFrameResources::GetRenderFinishedSemaphore(u32 imageIndex) const
    {
        return m_RenderFinishedSemaphores[imageIndex];
    }

    VkFence VulkanFrameResources::GetInFlightFence() const
    {
        return m_InFlightFence;
    }

    u32 VulkanFrameResources::GetRenderFinishedSemaphoreCount() const
    {
        return static_cast<u32>(m_RenderFinishedSemaphores.size());
    }
}
