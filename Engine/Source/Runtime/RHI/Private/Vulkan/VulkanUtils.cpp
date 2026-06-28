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

    VkImageType ToVulkanImageType(RHITextureDimension)
    {
        return VK_IMAGE_TYPE_2D;
    }

    VkImageViewType ToVulkanImageViewType(RHITextureViewDimension dimension)
    {
        switch (dimension)
        {
        case RHITextureViewDimension::Texture2DArray:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case RHITextureViewDimension::TextureCube:
            return VK_IMAGE_VIEW_TYPE_CUBE;
        case RHITextureViewDimension::Texture2D:
        default:
            return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    VkImageAspectFlags ToVulkanImageAspectFlags(RHITextureAspectFlags aspect)
    {
        VkImageAspectFlags flags = 0;
        if (HasFlag(aspect, RHITextureAspectFlags::Color)) { flags |= VK_IMAGE_ASPECT_COLOR_BIT; }
        if (HasFlag(aspect, RHITextureAspectFlags::Depth)) { flags |= VK_IMAGE_ASPECT_DEPTH_BIT; }
        if (HasFlag(aspect, RHITextureAspectFlags::Stencil)) { flags |= VK_IMAGE_ASPECT_STENCIL_BIT; }
        return flags;
    }

    VkBufferUsageFlags ToVulkanBufferUsageFlags(RHIBufferUsage usage)
    {
        VkBufferUsageFlags flags = 0;
        if (HasFlag(usage, RHIBufferUsage::Vertex)) { flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; }
        if (HasFlag(usage, RHIBufferUsage::Index)) { flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; }
        if (HasFlag(usage, RHIBufferUsage::Uniform)) { flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; }
        if (HasFlag(usage, RHIBufferUsage::Storage)) { flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
        if (HasFlag(usage, RHIBufferUsage::TransferSrc)) { flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT; }
        if (HasFlag(usage, RHIBufferUsage::TransferDst)) { flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; }
        return flags;
    }

    VmaMemoryUsage ToVmaMemoryUsage(RHIMemoryUsage usage)
    {
        switch (usage)
        {
        case RHIMemoryUsage::CPUToGPU:
            return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case RHIMemoryUsage::GPUToCPU:
            return VMA_MEMORY_USAGE_GPU_TO_CPU;
        case RHIMemoryUsage::GPUOnly:
        default:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        }
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

    VkDescriptorType ToVulkanDescriptorType(RHIBindingType type)
    {
        switch (type)
        {
        case RHIBindingType::CombinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case RHIBindingType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case RHIBindingType::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case RHIBindingType::SampledTexture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case RHIBindingType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        default:
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    VkShaderStageFlags ToVulkanShaderStageFlags(RHIShaderStageFlags flags)
    {
        VkShaderStageFlags result = 0;
        if (HasFlag(flags, RHIShaderStageFlags::Vertex)) { result |= VK_SHADER_STAGE_VERTEX_BIT; }
        if (HasFlag(flags, RHIShaderStageFlags::Fragment)) { result |= VK_SHADER_STAGE_FRAGMENT_BIT; }
        if (HasFlag(flags, RHIShaderStageFlags::Compute)) { result |= VK_SHADER_STAGE_COMPUTE_BIT; }
        return result;
    }

    VkShaderStageFlagBits ToVulkanShaderStage(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            return static_cast<VkShaderStageFlagBits>(0);
        }
    }
}
