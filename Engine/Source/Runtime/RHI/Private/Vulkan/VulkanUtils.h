#pragma once

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHITypes.h>

#include <volk.h>

#include <string>

namespace XEngine
{
    const char* VulkanResultToString(VkResult result);
    VkFormat RHIFormatToVulkanFormat(RHIFormat format);
    RHIFormat VulkanFormatToRHIFormat(VkFormat format);
}

#define XENGINE_VK_CHECK(expression)                                                                      \
    do                                                                                                    \
    {                                                                                                     \
        const VkResult result__ = (expression);                                                           \
        if (result__ != VK_SUCCESS)                                                                       \
        {                                                                                                 \
            std::string message__ = "Vulkan call failed: ";                                               \
            message__ += ::XEngine::VulkanResultToString(result__);                                       \
            XENGINE_LOG_ERROR(message__);                                                                 \
            XENGINE_ASSERT(false, "Vulkan call failed");                                                  \
        }                                                                                                 \
    } while (false)
