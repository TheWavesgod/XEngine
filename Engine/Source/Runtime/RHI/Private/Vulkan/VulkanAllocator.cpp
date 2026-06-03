#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION

#include "VulkanAllocator.h"

#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    VulkanAllocator::~VulkanAllocator()
    {
        Destroy();
    }

    bool VulkanAllocator::Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
    {
        if (m_Allocator != VK_NULL_HANDLE)
        {
            return true;
        }

        VmaVulkanFunctions vulkanFunctions {};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo createInfo {};
        createInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
        createInfo.physicalDevice = physicalDevice;
        createInfo.device = device;
        createInfo.instance = instance;
        createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        createInfo.pVulkanFunctions = &vulkanFunctions;

        const VkResult result = vmaCreateAllocator(&createInfo, &m_Allocator);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create VMA allocator: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        XENGINE_LOG_INFO("VMA allocator created");
        return true;
    }

    void VulkanAllocator::Destroy()
    {
        if (m_Allocator == VK_NULL_HANDLE)
        {
            return;
        }

        XENGINE_LOG_INFO("Destroying VMA allocator");
        vmaDestroyAllocator(m_Allocator);
        m_Allocator = VK_NULL_HANDLE;
    }

    VmaAllocator VulkanAllocator::GetHandle() const
    {
        return m_Allocator;
    }
}
