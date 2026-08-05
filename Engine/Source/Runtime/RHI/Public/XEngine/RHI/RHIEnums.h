// RHIEnums — central registry of RHI enumerations.
//
// Filled in incrementally:
//   M2:  RHIAdapterPreference, RHIAdapterType
//   M3:  RHIQueueType
//   M4:  RHIBufferUsage
//   M5:  RHIFormat, RHITextureUsage, RHITextureDimension,
//        RHIAddressMode, RHIFilterMode, RHICompareOp, RHIBorderColor
//   M6:  RHIPipelineStage, RHIAccessFlags, RHIImageLayout
//   M7:  RHIShaderStage
//   M10: RHIPresentMode
//
// Convention: all enums are `enum class : <small-int>` so the underlying
// type is fixed and ABI stable. Default values are 0 (None / first entry).

#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class RHIAdapterPreference : u8
    {
        Automatic,         // highest composite score (performance + power)
        HighPerformance,   // discrete / fastest GPU preferred
        LowPower,          // integrated / mobile GPU preferred
        Explicit,          // user picks by ID — M11 API, M2 only reserves the value
    };

    enum class RHIAdapterType : u8
    {
        Discrete,
        Integrated,
        CPU,
        Unknown,
    };

    enum class RHIQueueType : u8
    {
        Graphics,
        Compute,
        Transfer,
    };

    enum class RHIBufferUsage : u32
    {
        None        = 0,
        Vertex      = 1u << 0,
        Index       = 1u << 1,
        Uniform     = 1u << 2,
        Storage     = 1u << 3,
        TransferSrc = 1u << 4,
        TransferDst = 1u << 5,
        Indirect    = 1u << 6,
    };

    // M5: texture pixel format. u32 gives headroom for ~30 formats.
    enum class RHIFormat : u32
    {
        Unknown = 0,

        R8_UNORM,
        R8G8_UNORM,
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,

        R16_FLOAT,
        R16G16B16A16_FLOAT,

        R32_FLOAT,
        R32G32B32A32_FLOAT,

        D32_FLOAT,
        D24_UNORM_S8_UINT,

        BC1_RGB_UNORM,
        BC3_UNORM,
        BC5_UNORM,
        BC7_UNORM,
    };

    // M5: texture usage flags. u32 for 8+ flags.
    enum class RHITextureUsage : u32
    {
        None         = 0,
        ShaderRead   = 1u << 0,
        ShaderWrite  = 1u << 1,
        RenderTarget = 1u << 2,
        DepthStencil = 1u << 3,
        TransferSrc  = 1u << 4,
        TransferDst  = 1u << 5,
        Present      = 1u << 6,
    };

    // M5: texture dimensionality. u8 for 7 values.
    enum class RHITextureDimension : u8
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture1DArray,
        Texture2DArray,
        TextureCubeArray,
    };

    // M5: sampler address mode. 5 standard Vulkan modes.
    enum class RHIAddressMode : u8
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
        MirrorClampToEdge,
    };

    // M5: sampler filter mode. Nearest / Linear only.
    enum class RHIFilterMode : u8
    {
        Nearest,
        Linear,
    };

    // M5: depth / stencil comparison op. 8 standard Vulkan modes.
    enum class RHICompareOp : u8
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    // M5: sampler border color. 4 modes.
    enum class RHIBorderColor : u8
    {
        Black,
        White,
        TransparentBlack,
        TransparentWhite,
    };

    // M6: pipeline stage flags. u32. Used as src/dst in barriers.
    enum class RHIPipelineStage : u32
    {
        None                  = 0,
        TopOfPipe             = 1u << 0,
        DrawIndirect          = 1u << 1,
        VertexInput           = 1u << 2,
        VertexShader          = 1u << 3,
        FragmentShader        = 1u << 4,
        EarlyFragmentTests   = 1u << 5,
        LateFragmentTests    = 1u << 6,
        ColorAttachmentOutput = 1u << 7,
        ComputeShader         = 1u << 8,
        Transfer              = 1u << 9,
        BottomOfPipe          = 1u << 10,
        Host                  = 1u << 11,
        AllGraphics           = 1u << 12,
        AllCommands           = 1u << 13,
    };

    // M6: access flags. u32. Used as src/dst access in barriers.
    enum class RHIAccessFlags : u32
    {
        None                          = 0,
        IndirectCommandRead           = 1u << 0,
        IndexRead                     = 1u << 1,
        VertexAttributeRead           = 1u << 2,
        UniformRead                   = 1u << 3,
        InputAttachmentRead          = 1u << 4,
        ShaderRead                    = 1u << 5,
        ShaderWrite                   = 1u << 6,
        ColorAttachmentRead          = 1u << 7,
        ColorAttachmentWrite         = 1u << 8,
        DepthStencilAttachmentRead  = 1u << 9,
        DepthStencilAttachmentWrite = 1u << 10,
        TransferRead                  = 1u << 11,
        TransferWrite                 = 1u << 12,
        HostRead                      = 1u << 13,
        HostWrite                     = 1u << 14,
        MemoryRead                    = 1u << 15,
        MemoryWrite                   = 1u << 16,
    };

    // M6: image layout. Used in texture transitions.
    enum class RHIImageLayout : u32
    {
        Undefined,
        General,
        ColorAttachmentOptimal,
        DepthStencilAttachmentOptimal,
        DepthStencilReadOnlyOptimal,
        ShaderReadOnlyOptimal,
        TransferSrcOptimal,
        TransferDstOptimal,
        PresentSrc,
    };
}
