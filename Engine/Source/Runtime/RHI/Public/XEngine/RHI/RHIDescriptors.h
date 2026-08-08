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

    // RHIInstanceDesc — caller-controlled instance-level configuration.
    //
    // IMPORTANT: the first four fields are *positionally frozen*. Existing
    // call sites use C++20 designated initializers (see
    // Tests/Unit/RHI/Source/RHIInstanceTests.cpp and
    // Tests/Integration/VulkanRHI/Source/VulkanRHISkeleton.cpp). New fields
    // must be appended at the end so designated-initializer call sites
    // continue to compile.
    struct RHIInstanceDesc
    {
        std::string_view        ApplicationName    = "XEngineApp";
        u32                     ApplicationVersion = 1;
        bool                    EnableValidation   = false;
        bool                    EnableDebugMarkers = true;

        // Phase 2: validation-layer fine tuning (only effective when
        // EnableValidation == true). Backends may ignore fields they have
        // no equivalent for.
        RHIValidationSeverity   MinValidationSeverity = RHIValidationSeverity::Warning;
        bool                    EnableGPUAssistedValidation     = false;
        bool                    EnableSynchronizationValidation = false;

        // Phase 2: caller-preferred Vulkan instance API version. The backend
        // clamps this against vkEnumerateInstanceVersion.
        RHIApiVersion           PreferredApiVersion = RHIApiVersion::Version_1_3;
    };

    // RHIDeviceDesc — caller-controlled device-level configuration.
    //
    // Feature negotiation (see Docs/AI helper/plan.md §13 / M3):
    //   * RequiredFeatures: must be in RHIAdapter::GetSupportedFeatures();
    //     missing bits cause CreateDevice to return nullptr.
    //   * OptionalFeatures: requested-but-not-required. Silently downgraded
    //     if not supported; only enabled bits are reported by
    //     RHIDevice::GetEnabledFeatures().
    //
    // Invariant maintained by RHIInstance::CreateDevice's NVI wrapper:
    //   Required ⊆ Enabled ⊆ Supported.
    struct RHIDeviceDesc
    {
        RHIFeature         RequiredFeatures  = RHIFeature::None;
        RHIFeature         OptionalFeatures  = RHIFeature::None;
        u32                MaxFramesInFlight = 2;
        std::string_view   DebugName         = "";
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
