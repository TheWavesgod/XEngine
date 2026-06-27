#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <cstddef>

namespace XEngine
{
    class RHIBuffer;

    // V0 is blocking, single-threaded. No async / transfer queue.
    // The manager must not know about TextureAsset / MeshAsset / stb / glTF.
    class RHIUploadManager
    {
    public:
        virtual ~RHIUploadManager() = default;

        virtual void UploadBuffer(
            RHIBuffer& destination,
            const void* data,
            std::size_t size,
            std::size_t offset = 0) = 0;

        virtual void UploadTexture(
            RHITexture& destination,
            const void* data,
            std::size_t size,
            const RHITextureSubresourceRange& subresource = AllSubresources()) = 0;

        virtual void FlushUploads() = 0;

    protected:
        RHIUploadManager() = default;
    };
}