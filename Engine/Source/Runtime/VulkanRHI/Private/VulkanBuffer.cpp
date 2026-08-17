// VulkanBuffer — implementation.

#include "VulkanBuffer.h"
#include "VulkanDevice.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIFlags.h>

#include <cstring>
#include <string>

namespace XEngine
{
    namespace
    {
        // Translate the RHIBufferUsage bitmask to VkBufferUsageFlags.
        // Bits are OR-merged so callers can request a buffer that serves
        // multiple roles (e.g. STORAGE | TRANSFER_SRC).
        VkBufferUsageFlags TranslateUsageFlags(RHIBufferUsage usage)
        {
            VkBufferUsageFlags flags = 0;
            if (HasFlag(usage, RHIBufferUsage::Vertex))      flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            if (HasFlag(usage, RHIBufferUsage::Index))       flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            if (HasFlag(usage, RHIBufferUsage::Uniform))     flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (HasFlag(usage, RHIBufferUsage::Storage))     flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            if (HasFlag(usage, RHIBufferUsage::TransferSrc)) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            if (HasFlag(usage, RHIBufferUsage::TransferDst)) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            if (HasFlag(usage, RHIBufferUsage::Indirect))    flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            return flags;
        }
    }

    std::unique_ptr<VulkanBuffer> VulkanBuffer::Create(
        VulkanDevice& device,
        const RHIBufferDesc& desc)
    {
        // ValidateBufferDesc is enforced by the NVI wrapper on RHIDevice
        // before CreateBufferImpl is called, so the desc is already sane
        // (Size > 0, Usage != None). We still defensively re-check.
        if (desc.Size == 0 || desc.Usage == RHIBufferUsage::None)
        {
            return nullptr;
        }

        // The ctor does the VMA work. If vmaCreateBuffer fails, the
        // object's m_Buffer stays VK_NULL_HANDLE; we detect that below
        // and return nullptr. The partially-constructed object's
        // destructor is safe because it null-checks m_Buffer.
        std::unique_ptr<VulkanBuffer> buffer(new VulkanBuffer(device, desc));
        if (buffer->m_Buffer == VK_NULL_HANDLE)
        {
            return nullptr;
        }
        return buffer;
    }

    VulkanBuffer::VulkanBuffer(VulkanDevice& device, const RHIBufferDesc& desc)
        : RHIBuffer(device, device.GetBackend())
        , m_Size(desc.Size)
        , m_Usage(desc.Usage)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = desc.Size;
        bufferInfo.usage       = TranslateUsageFlags(desc.Usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

        // M4 policy: Uniform + TransferSrc are mapped host-visible so the
        // Map / Update API works without a staging buffer. Everything
        // else picks DEVICE_LOCAL (VMA's AUTO strategy picks the right
        // memory type per usage flag).
        const bool wantsHostVisible =
            HasFlag(desc.Usage, RHIBufferUsage::Uniform) ||
            HasFlag(desc.Usage, RHIBufferUsage::TransferSrc);
        if (wantsHostVisible)
        {
            allocInfo.flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            m_HostVisible = true;
        }

        VmaAllocator vma = device.GetVmaAllocator();
        VkResult result = vmaCreateBuffer(
            vma, &bufferInfo, &allocInfo,
            &m_Buffer, &m_Allocation, &m_AllocInfo);
        if (result != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR(
                std::string("VulkanBuffer::VulkanBuffer: vmaCreateBuffer failed ")
                + "(VkResult=" + std::to_string(result)
                + ", Size=" + std::to_string(desc.Size) + ")");
            m_Buffer     = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        if (m_Buffer == VK_NULL_HANDLE)
        {
            return;
        }
        // The owning device must still be alive when the buffer dies
        // (RHIDevice owns all its resources). m_OwnerDevice is set by
        // RHIBuffer(RHIDevice&, RHIBackend) base ctor; the original
        // reference was a VulkanDevice& so the upcast is safe.
        if (m_OwnerDevice != nullptr)
        {
            vmaDestroyBuffer(
                static_cast<VulkanDevice*>(m_OwnerDevice)->GetVmaAllocator(),
                m_Buffer,
                m_Allocation);
        }
        m_Buffer     = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
    }

    void VulkanBuffer::Update(u64 offset, const void* data, u64 size)
    {
        if (!m_HostVisible || m_Buffer == VK_NULL_HANDLE || data == nullptr || size == 0)
        {
            // M4: non-host-visible Update requires a staging buffer;
            // RHIUploadManager (M11) will provide this path.
            XENGINE_LOG_WARN(
                "VulkanBuffer::Update: ignored (non-host-visible or invalid args; "
                "M11 RHIUploadManager will provide staging-buffer path).");
            return;
        }
        vmaCopyMemoryToAllocation(
            static_cast<VulkanDevice*>(m_OwnerDevice)->GetVmaAllocator(),
            data, m_Allocation, offset, size);
    }

    void* VulkanBuffer::Map()
    {
        if (!m_HostVisible || m_Buffer == VK_NULL_HANDLE)
        {
            return nullptr;
        }
        void* mapped = nullptr;
        VkResult r = vmaMapMemory(
            static_cast<VulkanDevice*>(m_OwnerDevice)->GetVmaAllocator(),
            m_Allocation, &mapped);
        if (r != VK_SUCCESS)
        {
            return nullptr;
        }
        return mapped;
    }

    void VulkanBuffer::Unmap()
    {
        if (!m_HostVisible || m_Buffer == VK_NULL_HANDLE)
        {
            return;
        }
        vmaUnmapMemory(
            static_cast<VulkanDevice*>(m_OwnerDevice)->GetVmaAllocator(),
            m_Allocation);
    }
}
