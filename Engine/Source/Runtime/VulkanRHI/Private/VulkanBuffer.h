// VulkanBuffer — concrete RHIBuffer for the Vulkan backend.
//
// M4: wraps a VkBuffer + VmaAllocation pair. All M4 buffer usage flags
// (Vertex / Index / Uniform / Storage / TransferSrc / TransferDst /
// Indirect) are translated to the corresponding VkBufferUsageFlags and
// VMA picks the right memory type (HOST_VISIBLE for Uniform/TransferSrc,
// DEVICE_LOCAL otherwise). Map / Unmap / Update go through VMA so the
// host-visible mapping window is owned by the allocator.
//
// M11 will route DEFAULT-heap Update() through RHIUploadManager with a
// staging buffer; the M4 path for non-host-visible Update() is intentionally
// a no-op + warn (matches the contract: only host-visible buffers may be
// updated in-place at M4).

#pragma once

#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHIEnums.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <memory>

namespace XEngine
{
    class VulkanDevice;

    class VulkanBuffer : public RHIBuffer
    {
    public:
        // Used by XEngine::CheckedCast<T> to reject cross-backend casts.
        static constexpr RHIBackend ExpectedBackend = RHIBackend::Vulkan;

        // Backend factory. Returns nullptr on VkBuffer / Vma creation failure.
        // Constructed via private ctor — only this factory may produce one.
        static std::unique_ptr<VulkanBuffer> Create(
            VulkanDevice& device,
            const RHIBufferDesc& desc);

        // Public ctor — used by the static factory above. The base
        // RHIBuffer ctor is protected; we expose this derived ctor so
        // Create can `new VulkanBuffer(device, desc)` while keeping
        // the base RHIBuffer ctors protected.
        VulkanBuffer(VulkanDevice& device, const RHIBufferDesc& desc);

        ~VulkanBuffer() override;

        // RHIBuffer interface
        u64              GetSize()  const noexcept override { return m_Size; }
        RHIBufferUsage   GetUsage() const noexcept override { return m_Usage; }
        void             Update(u64 offset, const void* data, u64 size) override;
        void*            Map() override;
        void             Unmap() override;

        // Vulkan-specific accessors
        VkBuffer       GetVkBuffer()     const noexcept { return m_Buffer; }
        VmaAllocation  GetVmaAllocation() const noexcept { return m_Allocation; }
        bool           IsHostVisible()   const noexcept { return m_HostVisible; }

    private:
        // VkDevice + VmaAllocator are reached through the base owner's
        // VulkanDevice* via static_cast in the cpp; not duplicated here.

        VkBuffer         m_Buffer        = VK_NULL_HANDLE;
        VmaAllocation    m_Allocation    = VK_NULL_HANDLE;
        VmaAllocationInfo m_AllocInfo   {};
        u64              m_Size          = 0;
        RHIBufferUsage   m_Usage         = RHIBufferUsage::None;
        bool             m_HostVisible   = false;
    };
}
