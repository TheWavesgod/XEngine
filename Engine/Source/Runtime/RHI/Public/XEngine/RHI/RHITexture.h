// RHITexture + RHITextureView — GPU texture and its sub-resource view.
//
// M5 surface (lifecycle + query):
//   * GetFormat / GetDimension / GetWidth / GetHeight / GetDepth /
//     GetMipLevels / GetArrayLayers / GetUsage — query metadata
//   * Map / Update / etc. — NOT exposed at M5 (M11 RHIUploadManager takes over)
//
// M5 audit items addressed:
//   3.10 — RHITextureDimension includes TextureCubeArray
//   (Texture2DMS / Texture2DMSArray deferred to M11+)

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>

namespace XEngine
{
    // Forward declaration — RHITextureView's source pointer references it.
    class RHITexture;

    // RHITexture — GPU texture (long-lived or transient, both share this).
    // RHITexture does not expose Map/Update — uploads go through
    // RHIUploadManager (M11).
    class RHITexture : public RHIObject
    {
    public:
        virtual ~RHITexture() override = default;

        virtual RHIFormat          GetFormat()      const noexcept = 0;
        virtual RHITextureDimension GetDimension() const noexcept = 0;
        virtual u32                GetWidth()      const noexcept = 0;
        virtual u32                GetHeight()     const noexcept = 0;
        virtual u32                GetDepth()      const noexcept = 0;
        virtual u32                GetMipLevels()  const noexcept = 0;
        virtual u32                GetArrayLayers() const noexcept = 0;
        virtual RHITextureUsage    GetUsage()      const noexcept = 0;

        // Non-copyable / non-movable: backend textures wrap native handles
        // (VkImage, ID3D12Resource, id<MTLTexture>) that are not safely
        // copyable or movable.
        RHITexture(const RHITexture&) = delete;
        RHITexture& operator=(const RHITexture&) = delete;
        RHITexture(RHITexture&&) = delete;
        RHITexture& operator=(RHITexture&&) = delete;

    protected:
        explicit RHITexture(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHITexture(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };

    // RHITextureView — sub-resource interpretation of an RHITexture.
    //
    // Holds a back-pointer to the source (const, view never modifies source).
    // Lifetime is independent of source: the view may be destroyed before
    // or after the source texture, but reads through the view after the
    // source is destroyed are UB.
    class RHITextureView : public RHIObject
    {
    public:
        virtual ~RHITextureView() override = default;

        virtual RHIFormat          GetFormat()          const noexcept = 0;
        virtual const RHITexture*  GetSource()          const noexcept = 0;
        virtual RHITextureDimension GetDimension()     const noexcept = 0;
        virtual u32                GetBaseMipLevel()    const noexcept = 0;
        virtual u32                GetMipLevelCount()   const noexcept = 0;
        virtual u32                GetBaseArrayLayer()  const noexcept = 0;
        virtual u32                GetArrayLayerCount() const noexcept = 0;

        // Non-copyable / non-movable: backend views wrap native handles
        // (VkImageView, ID3D12View, id<MTLTexture>).
        RHITextureView(const RHITextureView&) = delete;
        RHITextureView& operator=(const RHITextureView&) = delete;
        RHITextureView(RHITextureView&&) = delete;
        RHITextureView& operator=(RHITextureView&&) = delete;

    protected:
        explicit RHITextureView(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHITextureView(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };
}
