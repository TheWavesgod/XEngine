#include "XEngine/RHI/RHIUtils.h"

namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend)
    {
        switch (backend)
        {
        case RHIBackend::None:   return "None";
        case RHIBackend::Vulkan: return "Vulkan";
        case RHIBackend::D3D12:  return "D3D12";
        case RHIBackend::Metal:  return "Metal";
        }
        return "Unknown";
    }

    bool IsDepthFormat(RHIFormat format)
    {
        return format == RHIFormat::D32Float;
    }

    bool IsStencilFormat(RHIFormat)
    {
        return false;
    }

    bool IsColorFormat(RHIFormat format)
    {
        return format != RHIFormat::Undefined && !IsDepthFormat(format);
    }

    RHITextureAspectFlags GetDefaultAspectForFormat(RHIFormat format)
    {
        return IsDepthFormat(format)
            ? RHITextureAspectFlags::Depth
            : RHITextureAspectFlags::Color;
    }

    u32 GetMaxMipLevels(u32 width, u32 height)
    {
        u32 extent = width > height ? width : height;
        u32 levels = 0;
        while (extent > 0)
        {
            ++levels;
            extent >>= 1u;
        }
        return levels;
    }

    bool IsTextureViewUsageCompatible(
        RHITextureUsageFlags textureUsage,
        RHITextureViewUsageFlags viewUsage)
    {
        if (viewUsage == RHITextureViewUsageFlags::None)
        {
            return false;
        }
        if (HasFlag(viewUsage, RHITextureViewUsageFlags::Sampled) &&
            !HasFlag(textureUsage, RHITextureUsageFlags::Sampled))
        {
            return false;
        }
        if (HasFlag(viewUsage, RHITextureViewUsageFlags::ColorAttachment) &&
            !HasFlag(textureUsage, RHITextureUsageFlags::ColorAttachment))
        {
            return false;
        }
        if (HasFlag(viewUsage, RHITextureViewUsageFlags::DepthAttachment) &&
            !HasFlag(textureUsage, RHITextureUsageFlags::DepthStencilAttachment))
        {
            return false;
        }
        return true;
    }

    bool NormalizeTextureViewDesc(
        const RHITextureDesc& textureDesc,
        const RHITextureViewDesc& viewDesc,
        RHITextureViewDesc& outDesc)
    {
        if (viewDesc.BaseMipLevel >= textureDesc.MipLevels ||
            viewDesc.BaseArrayLayer >= textureDesc.ArrayLayers)
        {
            return false;
        }

        outDesc = viewDesc;
        if (outDesc.MipCount == 0)
        {
            outDesc.MipCount = textureDesc.MipLevels - outDesc.BaseMipLevel;
        }
        if (outDesc.ArrayLayerCount == 0)
        {
            outDesc.ArrayLayerCount = textureDesc.ArrayLayers - outDesc.BaseArrayLayer;
        }
        if (outDesc.Format == RHIFormat::Undefined)
        {
            outDesc.Format = textureDesc.Format;
        }
        return true;
    }

    bool ValidateTextureDesc(
        const RHITextureDesc& desc,
        const RHICapabilities& capabilities)
    {
        if (desc.Width == 0 || desc.Height == 0 ||
            desc.MipLevels == 0 || desc.ArrayLayers == 0 ||
            desc.Format == RHIFormat::Undefined ||
            desc.Usage == RHITextureUsageFlags::None)
        {
            return false;
        }
        if (capabilities.MaxTextureDimension2D > 0 &&
            (desc.Width > capabilities.MaxTextureDimension2D ||
             desc.Height > capabilities.MaxTextureDimension2D))
        {
            return false;
        }
        if (capabilities.MaxTextureArrayLayers > 0 &&
            desc.ArrayLayers > capabilities.MaxTextureArrayLayers)
        {
            return false;
        }
        if (desc.MipLevels > GetMaxMipLevels(desc.Width, desc.Height))
        {
            return false;
        }
        if (desc.Dimension == RHITextureDimension::Texture2D && desc.ArrayLayers != 1)
        {
            return false;
        }
        if (desc.Dimension == RHITextureDimension::TextureCube &&
            (desc.ArrayLayers != 6 || desc.Width != desc.Height))
        {
            return false;
        }
        if (IsDepthFormat(desc.Format) &&
            HasFlag(desc.Usage, RHITextureUsageFlags::ColorAttachment))
        {
            return false;
        }
        if (IsColorFormat(desc.Format) &&
            HasFlag(desc.Usage, RHITextureUsageFlags::DepthStencilAttachment))
        {
            return false;
        }
        return true;
    }

    bool ValidateTextureViewDesc(
        const RHITextureDesc& textureDesc,
        const RHITextureViewDesc& viewDesc,
        const RHICapabilities&)
    {
        if (viewDesc.Texture == nullptr ||
            viewDesc.MipCount == 0 || viewDesc.ArrayLayerCount == 0 ||
            viewDesc.BaseMipLevel >= textureDesc.MipLevels ||
            viewDesc.MipCount > textureDesc.MipLevels - viewDesc.BaseMipLevel ||
            viewDesc.BaseArrayLayer >= textureDesc.ArrayLayers ||
            viewDesc.ArrayLayerCount > textureDesc.ArrayLayers - viewDesc.BaseArrayLayer ||
            viewDesc.Format != textureDesc.Format ||
            !IsTextureViewUsageCompatible(textureDesc.Usage, viewDesc.Usage))
        {
            return false;
        }

        const RHITextureAspectFlags expectedAspect = GetDefaultAspectForFormat(textureDesc.Format);
        if (viewDesc.Aspect != expectedAspect)
        {
            return false;
        }
        if (viewDesc.ViewDimension == RHITextureViewDimension::Texture2D &&
            viewDesc.ArrayLayerCount != 1)
        {
            return false;
        }
        if (viewDesc.ViewDimension == RHITextureViewDimension::Texture2DArray &&
            textureDesc.Dimension == RHITextureDimension::Texture2D)
        {
            return false;
        }
        if (viewDesc.ViewDimension == RHITextureViewDimension::TextureCube &&
            (textureDesc.Dimension != RHITextureDimension::TextureCube ||
             viewDesc.ArrayLayerCount != 6 ||
             viewDesc.BaseArrayLayer % 6 != 0))
        {
            return false;
        }
        return true;
    }
}
