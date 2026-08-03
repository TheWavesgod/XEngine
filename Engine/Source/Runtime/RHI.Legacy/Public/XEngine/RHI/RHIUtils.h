#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend);
    bool IsDepthFormat(RHIFormat format);
    bool IsStencilFormat(RHIFormat format);
    bool IsColorFormat(RHIFormat format);
    RHITextureAspectFlags GetDefaultAspectForFormat(RHIFormat format);
    u32 GetMaxMipLevels(u32 width, u32 height);

    bool IsTextureViewUsageCompatible(
        RHITextureUsageFlags textureUsage,
        RHITextureViewUsageFlags viewUsage);

    bool NormalizeTextureViewDesc(
        const RHITextureDesc& textureDesc,
        const RHITextureViewDesc& viewDesc,
        RHITextureViewDesc& outDesc);

    bool ValidateTextureDesc(
        const RHITextureDesc& desc,
        const RHICapabilities& capabilities);

    bool ValidateTextureViewDesc(
        const RHITextureDesc& textureDesc,
        const RHITextureViewDesc& viewDesc,
        const RHICapabilities& capabilities);
}
