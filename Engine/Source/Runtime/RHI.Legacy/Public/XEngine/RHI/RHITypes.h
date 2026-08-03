#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <cstddef>
#include <string>
#include <vector>

namespace XEngine
{
    class RHIBindGroupLayout;
    class RHIBuffer;
    class RHISampler;
    class RHIShader;
    class RHITexture;
    class RHITextureView;

    enum class RHIBackend : u8
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

    enum class RHIBufferUsage : u32
    {
        None = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5
    };

    inline RHIBufferUsage operator|(RHIBufferUsage lhs, RHIBufferUsage rhs)
    {
        return static_cast<RHIBufferUsage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
    }

    inline bool HasFlag(RHIBufferUsage value, RHIBufferUsage flag)
    {
        return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
    }

    enum class RHIMemoryUsage
    {
        GPUOnly,
        CPUToGPU,
        GPUToCPU
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
        Texture2DArray,
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

    enum class RHITextureViewDimension : u8
    {
        Texture2D,
        Texture2DArray,
        TextureCube
    };

    enum class RHITextureViewUsageFlags : u32
    {
        None = 0,
        Sampled = 1 << 0,
        ColorAttachment = 1 << 1,
        DepthAttachment = 1 << 2
    };

    inline RHITextureViewUsageFlags operator|(
        RHITextureViewUsageFlags lhs,
        RHITextureViewUsageFlags rhs)
    {
        return static_cast<RHITextureViewUsageFlags>(
            static_cast<u32>(lhs) | static_cast<u32>(rhs));
    }

    inline bool HasFlag(RHITextureViewUsageFlags value, RHITextureViewUsageFlags flag)
    {
        return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
    }

    enum class RHITextureAspectFlags : u8
    {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        DepthStencil = Depth | Stencil,
        MetaData = 1 << 3
    };

    inline RHITextureAspectFlags operator|(
        RHITextureAspectFlags lhs,
        RHITextureAspectFlags rhs)
    {
        return static_cast<RHITextureAspectFlags>(
            static_cast<u8>(lhs) | static_cast<u8>(rhs));
    }

    inline bool HasFlag(RHITextureAspectFlags value, RHITextureAspectFlags flag)
    {
        return (static_cast<u8>(value) & static_cast<u8>(flag)) != 0;
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

    inline RHIShaderStageFlags operator|(
        RHIShaderStageFlags lhs,
        RHIShaderStageFlags rhs)
    {
        return static_cast<RHIShaderStageFlags>(
            static_cast<u32>(lhs) | static_cast<u32>(rhs));
    }

    inline RHIShaderStageFlags operator&(
        RHIShaderStageFlags lhs,
        RHIShaderStageFlags rhs)
    {
        return static_cast<RHIShaderStageFlags>(
            static_cast<u32>(lhs) & static_cast<u32>(rhs));
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

    struct RHICapabilities
    {
        u32 MaxTextureDimension2D = 0;
        u32 MaxTextureArrayLayers = 0;
        u32 MaxPushConstantSize = 0;
        u32 MaxBoundDescriptorSets = 0;
        bool SupportsSamplerAnisotropy = false;
        f32 MaxSamplerAnisotropy = 1.0f;
        bool SupportsDynamicRendering = false;
    };

    struct RHIBufferDesc
    {
        std::size_t Size = 0;
        RHIBufferUsage Usage = RHIBufferUsage::None;
        RHIMemoryUsage MemoryUsage = RHIMemoryUsage::GPUOnly;
        const char* DebugName = nullptr;
    };

    struct RHITextureDesc
    {
        u32 Width = 1;
        u32 Height = 1;
        u32 MipLevels = 1;
        u32 ArrayLayers = 1;
        RHIFormat Format = RHIFormat::RGBA8Unorm;
        RHITextureDimension Dimension = RHITextureDimension::Texture2D;
        RHITextureUsageFlags Usage =
            RHITextureUsageFlags::Sampled | RHITextureUsageFlags::TransferDst;
        bool GenerateMips = false;
        const char* DebugName = nullptr;
    };

    struct RHITextureSubresourceRange
    {
        u32 BaseMipLevel = 0;
        u32 MipCount = 0;
        u32 BaseArrayLayer = 0;
        u32 ArrayLayerCount = 0;
    };

    inline RHITextureSubresourceRange AllSubresources()
    {
        return { 0, 0, 0, 0 };
    }

    struct RHITextureViewDesc
    {
        RHITexture* Texture = nullptr;
        RHITextureViewUsageFlags Usage = RHITextureViewUsageFlags::Sampled;
        RHITextureViewDimension ViewDimension = RHITextureViewDimension::Texture2D;
        RHITextureAspectFlags Aspect = RHITextureAspectFlags::Color;
        RHIFormat Format = RHIFormat::Undefined;
        u32 BaseMipLevel = 0;
        u32 MipCount = 1;
        u32 BaseArrayLayer = 0;
        u32 ArrayLayerCount = 1;
        const char* DebugName = nullptr;
    };

    struct RHISamplerDesc
    {
        RHIFilter MinFilter = RHIFilter::Linear;
        RHIFilter MagFilter = RHIFilter::Linear;
        RHIAddressMode AddressU = RHIAddressMode::Repeat;
        RHIAddressMode AddressV = RHIAddressMode::Repeat;
        RHIAddressMode AddressW = RHIAddressMode::Repeat;
        f32 MaxAnisotropy = 1.0f;
        const char* DebugName = nullptr;
    };

    struct RHIBindGroupLayoutEntry
    {
        u32 Binding = 0;
        RHIBindingType Type = RHIBindingType::Unknown;
        RHIShaderStageFlags Visibility = RHIShaderStageFlags::Fragment;
        u32 Count = 1;
    };

    struct RHIBindGroupLayoutDesc
    {
        std::vector<RHIBindGroupLayoutEntry> Entries;
        const char* DebugName = nullptr;
    };

    struct RHIBindingResource
    {
        u32 Binding = 0;
        RHIBindingType Type = RHIBindingType::Unknown;
        RHITextureView* TextureView = nullptr;
        RHISampler* Sampler = nullptr;
        RHIBuffer* Buffer = nullptr;
        u64 BufferOffset = 0;
        u64 BufferSize = 0;
    };

    struct RHIBindGroupDesc
    {
        RHIBindGroupLayout* Layout = nullptr;
        std::vector<RHIBindingResource> Resources;
        const char* DebugName = nullptr;
    };

    struct RHIVertexAttributeDesc
    {
        u32 Location = 0;
        RHIFormat Format = RHIFormat::Undefined;
        u32 Offset = 0;
    };

    struct RHIVertexBufferLayoutDesc
    {
        u32 Stride = 0;
        std::vector<RHIVertexAttributeDesc> Attributes;
    };

    struct RHIGraphicsPipelineDesc
    {
        RHIShader* VertexShader = nullptr;
        RHIShader* FragmentShader = nullptr;
        RHIFormat ColorFormat = RHIFormat::Undefined;
        RHIFormat DepthFormat = RHIFormat::Undefined;
        bool HasColorAttachment = true;
        bool EnableDepthTest = true;
        bool EnableDepthWrite = true;
        bool EnableDepthBias = false;
        f32 DepthBiasConstantFactor = 0.0f;
        f32 DepthBiasClamp = 0.0f;
        f32 DepthBiasSlopeFactor = 0.0f;
        RHIVertexBufferLayoutDesc VertexLayout;
        std::vector<RHIBindGroupLayout*> BindGroupLayouts;
        u32 PushConstantSize = 0;
        RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;
        const char* DebugName = nullptr;
    };

    struct RHIShaderDesc
    {
        ShaderStage Stage = ShaderStage::Unknown;
        ShaderTarget Target = ShaderTarget::Unknown;
        ShaderCodeFormat Format = ShaderCodeFormat::Unknown;
        std::string EntryPoint;
        const u8* Code = nullptr;
        std::size_t CodeSize = 0;
        const char* DebugName = nullptr;
    };

    struct RHIRenderOutputDesc
    {
        RHITextureView* ColorTargetView = nullptr;
        RHITextureView* DepthTargetView = nullptr;
        RHIRect2D Viewport {};
        RHIFormat ColorFormat = RHIFormat::Undefined;
        RHIFormat DepthFormat = RHIFormat::Undefined;
        bool RenderToSwapchain = true;
    };

    struct RHIPhysicalDeviceInfo
    {
        const char* Name = "";
        u32 VendorId = 0;
        u32 DeviceId = 0;
    };
}
