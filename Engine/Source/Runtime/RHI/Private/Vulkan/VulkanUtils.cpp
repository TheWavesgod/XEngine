#include "VulkanUtils.h"

namespace XEngine
{
    const char* VulkanResultToString(VkResult result)
    {
        switch (result)
        {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        default:
            return "VK_UNKNOWN";
        }
    }

    VkFormat RHIFormatToVulkanFormat(RHIFormat format)
    {
        switch (format)
        {
        case RHIFormat::RGBA8Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case RHIFormat::RGBA8Srgb:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case RHIFormat::BGRA8Unorm:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case RHIFormat::BGRA8Srgb:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case RHIFormat::RGBA16Float:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case RHIFormat::RGBA32Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case RHIFormat::D32Float:
            return VK_FORMAT_D32_SFLOAT;
        case RHIFormat::R32G32Float:
            return VK_FORMAT_R32G32_SFLOAT;
        case RHIFormat::R32G32B32Float:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case RHIFormat::R32G32B32A32Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            return VK_FORMAT_UNDEFINED;
        }
    }

    RHIFormat VulkanFormatToRHIFormat(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return RHIFormat::RGBA8Unorm;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return RHIFormat::RGBA8Srgb;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return RHIFormat::BGRA8Unorm;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return RHIFormat::BGRA8Srgb;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return RHIFormat::RGBA16Float;
        case VK_FORMAT_D32_SFLOAT:
            return RHIFormat::D32Float;
        case VK_FORMAT_R32G32_SFLOAT:
            return RHIFormat::R32G32Float;
        case VK_FORMAT_R32G32B32_SFLOAT:
            return RHIFormat::R32G32B32Float;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return RHIFormat::R32G32B32A32Float;
        default:
            return RHIFormat::Undefined;
        }
    }

    VkImageUsageFlags ToVulkanImageUsageFlags(RHITextureUsageFlags usage)
    {
        VkImageUsageFlags flags = 0;
        if (HasFlag(usage, RHITextureUsageFlags::Sampled)) { flags |= VK_IMAGE_USAGE_SAMPLED_BIT; }
        if (HasFlag(usage, RHITextureUsageFlags::ColorAttachment)) { flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; }
        if (HasFlag(usage, RHITextureUsageFlags::DepthStencilAttachment)) { flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }
        if (HasFlag(usage, RHITextureUsageFlags::Storage)) { flags |= VK_IMAGE_USAGE_STORAGE_BIT; }
        if (HasFlag(usage, RHITextureUsageFlags::TransferSrc)) { flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; }
        if (HasFlag(usage, RHITextureUsageFlags::TransferDst)) { flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; }
        return flags;
    }

    VkFilter ToVulkanFilter(RHIFilter filter)
    {
        switch (filter)
        {
        case RHIFilter::Nearest:
            return VK_FILTER_NEAREST;
        case RHIFilter::Linear:
        default:
            return VK_FILTER_LINEAR;
        }
    }

    VkSamplerAddressMode ToVulkanAddressMode(RHIAddressMode mode)
    {
        switch (mode)
        {
        case RHIAddressMode::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case RHIAddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case RHIAddressMode::ClampToBorder:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case RHIAddressMode::Repeat:
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }
}
