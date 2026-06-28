#pragma once

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHITypes.h>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <string>

namespace XEngine
{
    const char* VulkanResultToString(VkResult result);
    VkFormat RHIFormatToVulkanFormat(RHIFormat format);
    RHIFormat VulkanFormatToRHIFormat(VkFormat format);
    VkImageUsageFlags ToVulkanImageUsageFlags(RHITextureUsageFlags usage);
    VkImageType ToVulkanImageType(RHITextureDimension dimension);
    VkImageViewType ToVulkanImageViewType(RHITextureViewDimension dimension);
    VkImageAspectFlags ToVulkanImageAspectFlags(RHITextureAspectFlags aspect);
    VkBufferUsageFlags ToVulkanBufferUsageFlags(RHIBufferUsage usage);
    VmaMemoryUsage ToVmaMemoryUsage(RHIMemoryUsage usage);
    VkFilter ToVulkanFilter(RHIFilter filter);
    VkSamplerAddressMode ToVulkanAddressMode(RHIAddressMode mode);
    VkDescriptorType ToVulkanDescriptorType(RHIBindingType type);
    VkShaderStageFlags ToVulkanShaderStageFlags(RHIShaderStageFlags flags);
    VkShaderStageFlagBits ToVulkanShaderStage(ShaderStage stage);
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
