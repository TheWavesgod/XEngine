// RHIBuffer — GPU buffer resource.
//
// M4 surface (lifecycle + readback / update):
//   * GetSize() / GetUsage()       — query metadata
//   * Update(offset, data, size)   — HOST_VISIBLE in-place write
//   * Map() / Unmap()              — explicit map for readback / pinning
//
// M4 backend requirements (Vulkan):
//   - VMA allocation sized by Size
//   - VkBuffer + VkBufferCreateInfo driven by Usage flags
//   - HOST_VISIBLE picked for Uniform/TransferSrc; DEVICE_LOCAL otherwise
//
// M11 will replace Update with RHIUploadManager + staging buffer.
// At that point M11 may keep Update as the underlying channel or deprecate it.

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>

namespace XEngine
{
    // Forward declaration — RHIBuffer's constructor takes RHIDevice&.
    class RHIDevice;

    // RHIBuffer — GPU buffer.
    class RHIBuffer : public RHIObject
    {
    public:
        virtual ~RHIBuffer() override = default;

        // Query metadata.
        virtual u64 GetSize() const noexcept = 0;
        virtual RHIBufferUsage GetUsage() const noexcept = 0;

        // M4: in-place write for HOST_VISIBLE buffers. For DEVICE_LOCAL this
        // is a no-op or fall back to a staging buffer if the backend supports it.
        // M11 will replace this with RHIUploadManager.
        virtual void Update(u64 offset, const void* data, u64 size) = 0;

        // M4: explicit Map for readback / driver pinning. Returns nullptr
        // if the buffer is not HOST_VISIBLE; callers must Unmap() after use.
        virtual void* Map() = 0;
        virtual void Unmap() = 0;

        // Non-copyable / non-movable: backend buffers wrap native handles
        // (VkBuffer, ID3D12Resource, id<MTLBuffer>) that are not safely
        // copyable or movable.
        RHIBuffer(const RHIBuffer&) = delete;
        RHIBuffer& operator=(const RHIBuffer&) = delete;
        RHIBuffer(RHIBuffer&&) = delete;
        RHIBuffer& operator=(RHIBuffer&&) = delete;

    protected:
        explicit RHIBuffer(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHIBuffer(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };
}
