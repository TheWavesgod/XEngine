// RHIDescriptors — central registry of resource creation parameter structs.
//
// Filled in incrementally:
//   M2:  RHIInstanceDesc, RHIDeviceDesc
//   M4:  RHIBufferDesc
//   M5:  RHITextureDesc, RHITextureViewDesc, RHISamplerDesc
//   M6:  RHIFenceDesc, RHISemaphoreDesc, RHICommandListDesc
//   M7:  RHIShaderDesc, RHIBindGroupLayoutDesc, RHIBindGroupDesc
//   M8:  RHIComputePipelineDesc
//   M9:  RHIGraphicsPipelineDesc
//   M10: RHISwapchainDesc
//
// Convention: each desc is a POD struct with sensible defaults so empty
// braces work as a default. `DebugName` is added per-desc as needed.
// Validation lives in RHIValidation.h.

#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIEnums.h>

#include <string_view>

namespace XEngine
{
    class RHITexture;  // forward — TextureViewDesc references it

    struct RHIInstanceDesc
    {
        std::string_view ApplicationName    = "XEngineApp";
        u32              ApplicationVersion = 1;
        bool             EnableValidation   = false;
        bool             EnableDebugMarkers = true;
    };

    struct RHIDeviceDesc
    {
    };

    struct RHIBufferDesc
    {
        u64              Size      = 0;
        RHIBufferUsage   Usage     = RHIBufferUsage::None;
        std::string_view DebugName = "";
    };

    struct RHITextureDesc
    {
        RHITextureDimension Dimension   = RHITextureDimension::Texture2D;
        u32                Width       = 0;
        u32                Height      = 0;
        u32                Depth       = 1;
        u32                MipLevels   = 1;
        u32                ArrayLayers = 1;
        RHIFormat          Format      = RHIFormat::Unknown;
        RHITextureUsage    Usage       = RHITextureUsage::None;
        std::string_view   DebugName   = "";
    };

    struct RHITextureViewDesc
    {
        const RHITexture*  Source          = nullptr;
        RHITextureDimension Dimension       = RHITextureDimension::Texture2D;
        RHIFormat          Format          = RHIFormat::Unknown;
        u32                BaseMipLevel    = 0;
        u32                MipLevelCount   = 0;
        u32                BaseArrayLayer  = 0;
        u32                ArrayLayerCount = 0;
    };

    struct RHISamplerDesc
    {
        RHIAddressMode AddressModeU = RHIAddressMode::Repeat;
        RHIAddressMode AddressModeV = RHIAddressMode::Repeat;
        RHIAddressMode AddressModeW = RHIAddressMode::Repeat;
        RHIFilterMode  MagFilter    = RHIFilterMode::Nearest;
        RHIFilterMode  MinFilter    = RHIFilterMode::Nearest;
        RHIFilterMode  MipFilter    = RHIFilterMode::Nearest;
        float          LodBias      = 0.0f;
        u32            MaxAnisotropy = 1;
        bool           CompareEnable = false;
        RHICompareOp   CompareOp    = RHICompareOp::Never;
        float          MinLod        = 0.0f;
        float          MaxLod        = 1.0f;
        RHIBorderColor BorderColor   = RHIBorderColor::Black;
    };

    // M6: parameters for creating a fence (CPU-GPU sync).
    struct RHIFenceDesc
    {
        bool InitialSignaled = false;
    };

    // M6: parameters for creating a semaphore (GPU-GPU sync).
    // Empty for M6; M7+ timeline semaphores will add type / initialValue.
    struct RHISemaphoreDesc
    {
    };

    // M6: parameters for creating a command list.
    struct RHICommandListDesc
    {
        RHIQueueType    TargetQueue = RHIQueueType::Graphics;
        std::string_view DebugName  = "";
    };
}
