// RHIValidation — central registry of desc validation functions.
//
// Each function returns XEngine::Result (success or failure with message).
// Called by NVI wrappers on RHIDevice / RHIInstance / etc. to centralize
// parameter validation rules in one place, separately testable.
//
// Filled in incrementally:
//   M4: ValidateBufferDesc
//   M5: ValidateTextureDesc, ValidateTextureViewDesc, ValidateSamplerDesc
//   M6: ValidateFenceDesc, ValidateSemaphoreDesc, ValidateCommandListDesc
//   M7: ValidateShaderDesc, ValidateBindGroupLayoutDesc, ValidateBindGroupDesc
//   M10: ValidateSwapchainDesc
//
// Why not folds into RHIDescriptors.h: keeps descriptor definitions (pure
// data) separate from validation logic (behavior with side effects).

#pragma once

#include <XEngine/Core/Result.h>

#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>

namespace XEngine
{
    // M4: validates a buffer creation descriptor.
    inline Result ValidateBufferDesc(const RHIBufferDesc& desc)
    {
        if (desc.Size == 0)
        {
            return Result::Failure("RHIBufferDesc::Size must be > 0");
        }
        if (desc.Usage == RHIBufferUsage::None)
        {
            return Result::Failure("RHIBufferDesc::Usage must not be None");
        }
        return Result::Ok();
    }

    // M5: validates a texture creation descriptor.
    inline Result ValidateTextureDesc(const RHITextureDesc& desc)
    {
        if (desc.Format == RHIFormat::Unknown)
        {
            return Result::Failure("RHITextureDesc::Format must not be Unknown");
        }
        if (desc.Usage == RHITextureUsage::None)
        {
            return Result::Failure("RHITextureDesc::Usage must not be None");
        }
        if (desc.MipLevels == 0)
        {
            return Result::Failure("RHITextureDesc::MipLevels must be > 0");
        }
        if (desc.ArrayLayers == 0)
        {
            return Result::Failure("RHITextureDesc::ArrayLayers must be > 0");
        }
        if (desc.Width == 0)
        {
            return Result::Failure("RHITextureDesc::Width must be > 0");
        }
        if (desc.Height == 0 && desc.Dimension != RHITextureDimension::Texture1D)
        {
            return Result::Failure("RHITextureDesc::Height must be > 0 for non-1D dimensions");
        }
        if (desc.Depth == 0 && desc.Dimension == RHITextureDimension::Texture3D)
        {
            return Result::Failure("RHITextureDesc::Depth must be > 0 for 3D textures");
        }
        return Result::Ok();
    }

    // M5: validates a texture view descriptor.
    inline Result ValidateTextureViewDesc(const RHITextureViewDesc& desc)
    {
        if (desc.Source == nullptr)
        {
            return Result::Failure("RHITextureViewDesc::Source must not be null");
        }
        if (desc.Format == RHIFormat::Unknown)
        {
            return Result::Failure("RHITextureViewDesc::Format must not be Unknown");
        }
        return Result::Ok();
    }

    // M5: validates a sampler descriptor.
    inline Result ValidateSamplerDesc(const RHISamplerDesc& desc)
    {
        (void)desc;
        return Result::Ok();
    }

    // M6: validates a fence descriptor. Always succeeds for M6.
    inline Result ValidateFenceDesc(const RHIFenceDesc& desc)
    {
        (void)desc;
        return Result::Ok();
    }

    // M6: validates a semaphore descriptor. Always succeeds for M6.
    inline Result ValidateSemaphoreDesc(const RHISemaphoreDesc& desc)
    {
        (void)desc;
        return Result::Ok();
    }

    // M6: validates a command list descriptor. Always succeeds for M6.
    inline Result ValidateCommandListDesc(const RHICommandListDesc& desc)
    {
        (void)desc;
        return Result::Ok();
    }
}
