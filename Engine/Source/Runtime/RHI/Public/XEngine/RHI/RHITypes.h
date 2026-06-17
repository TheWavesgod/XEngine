#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class RHIBackend
    {
        None,
        Vulkan,
        D3D12,
        Metal
    };

    enum class RHIQueueType
    {
        Graphics,
        Compute,
        Transfer
    };

    enum class RHIFormat
    {
        Undefined,
        RGBA8Unorm,
        RGBA8Srgb,
        BGRA8Unorm,
        BGRA8Srgb,
        RGBA16Float,
        RGBA32Float,
        D32Float,
        R32G32Float,
        R32G32B32Float,
        R32G32B32A32Float
    };

    enum class RHIIndexFormat
    {
        UInt16,
        UInt32
    };

    enum class RHIFilter
    {
        Nearest,
        Linear
    };

    enum class RHIAddressMode
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class RHITextureDimension
    {
        Texture2D,
        TextureCube
    };

    enum class RHITextureUsageFlags : u32
    {
        None = 0,
        Sampled = 1 << 0,
        ColorAttachment = 1 << 1,
        DepthStencilAttachment = 1 << 2,
        Storage = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5
    };

    inline RHITextureUsageFlags operator|(RHITextureUsageFlags lhs, RHITextureUsageFlags rhs)
    {
        return static_cast<RHITextureUsageFlags>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
    }

    inline RHITextureUsageFlags operator&(RHITextureUsageFlags lhs, RHITextureUsageFlags rhs)
    {
        return static_cast<RHITextureUsageFlags>(static_cast<u32>(lhs) & static_cast<u32>(rhs));
    }

    inline bool HasFlag(RHITextureUsageFlags value, RHITextureUsageFlags flag)
    {
        return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
    }

    enum class RHIBindingType
    {
        Unknown,
        UniformBuffer,
        StorageBuffer,
        SampledTexture,
        Sampler,
        CombinedImageSampler
    };

    enum class RHIShaderStageFlags : u32
    {
        None = 0,
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2,
        AllGraphics = Vertex | Fragment,
        All = Vertex | Fragment | Compute
    };

    inline RHIShaderStageFlags operator|(RHIShaderStageFlags lhs, RHIShaderStageFlags rhs)
    {
        return static_cast<RHIShaderStageFlags>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
    }

    inline RHIShaderStageFlags operator&(RHIShaderStageFlags lhs, RHIShaderStageFlags rhs)
    {
        return static_cast<RHIShaderStageFlags>(static_cast<u32>(lhs) & static_cast<u32>(rhs));
    }

    inline bool HasFlag(RHIShaderStageFlags value, RHIShaderStageFlags flag)
    {
        return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
    }

    struct RHIColor
    {
        f32 R = 0.0f;
        f32 G = 0.0f;
        f32 B = 0.0f;
        f32 A = 1.0f;
    };

    struct RHIRect2D
    {
        u32 X = 0;
        u32 Y = 0;
        u32 Width = 0;
        u32 Height = 0;
    };

    class RHITexture;

    struct RHIRenderOutputDesc
    {
        RHITexture* ColorTarget = nullptr;
        RHITexture* DepthTarget = nullptr;
        RHIRect2D Viewport {};
        RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
        RHIFormat DepthFormat = RHIFormat::D32Float;
        bool RenderToSwapchain = true;
    };

    struct RHIPhysicalDeviceInfo
    {
        const char* Name = "";
        u32 VendorId = 0;
        u32 DeviceId = 0;
    };
}
