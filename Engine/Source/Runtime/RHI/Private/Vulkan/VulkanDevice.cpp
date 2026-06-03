#include "VulkanDevice.h"

#include <XEngine/Logging/Log.h>

namespace XEngine
{
    VulkanDevice::VulkanDevice()
    {
        XENGINE_LOG_INFO("VulkanDevice skeleton created");
    }

    VulkanDevice::~VulkanDevice()
    {
        XENGINE_LOG_INFO("VulkanDevice skeleton destroyed");
    }

    RHIBackend VulkanDevice::GetBackend() const
    {
        return RHIBackend::Vulkan;
    }

    void VulkanDevice::WaitIdle()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        // TODO(Stage 2B): Call vkDeviceWaitIdle after a real VkDevice exists.
    }
}
