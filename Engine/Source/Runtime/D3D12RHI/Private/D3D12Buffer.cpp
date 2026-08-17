// D3D12Buffer — implementation.

#include "D3D12Buffer.h"
#include "D3D12Device.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIFlags.h>

#include <cstring>
#include <string>

namespace XEngine
{
    namespace
    {
        // D3D12 requires committed buffer resources to be aligned to
        // 64 KiB. Round up so any D3D12 resource created via
        // CreateCommittedResource meets the alignment contract.
        constexpr UINT64 kBufferAlignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        UINT64 AlignUp(UINT64 value, UINT64 alignment)
        {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        // Translate RHIBufferUsage to D3D12_RESOURCE_FLAGS.
        // Bits are OR-merged for multi-role buffers.
        D3D12_RESOURCE_FLAGS TranslateResourceFlags(RHIBufferUsage usage)
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            if (HasFlag(usage, RHIBufferUsage::Storage))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            return flags;
        }
    }

    std::unique_ptr<D3D12Buffer> D3D12Buffer::Create(
        D3D12Device& device,
        const RHIBufferDesc& desc)
    {
        // Defensive re-check; NVI wrapper already enforced these.
        if (desc.Size == 0 || desc.Usage == RHIBufferUsage::None)
        {
            return nullptr;
        }

        std::unique_ptr<D3D12Buffer> buffer(new D3D12Buffer(device, desc));
        if (buffer->m_Resource == nullptr)
        {
            return nullptr;
        }
        return buffer;
    }

    D3D12Buffer::D3D12Buffer(D3D12Device& device, const RHIBufferDesc& desc)
        : RHIBuffer(device, device.GetBackend())
        , m_Size(desc.Size)
        , m_Usage(desc.Usage)
    {
        // M4 policy: Uniform / TransferSrc go to UPLOAD heap so the
        // Map / Update API is a direct memcpy with no staging buffer.
        // Everything else (Vertex, Index, Storage, TransferDst, Indirect)
        // goes to DEFAULT heap; Update on DEFAULT will be wired in M11
        // via RHIUploadManager.
        const bool wantsHostVisible =
            HasFlag(desc.Usage, RHIBufferUsage::Uniform) ||
            HasFlag(desc.Usage, RHIBufferUsage::TransferSrc);
        m_HeapType = wantsHostVisible
            ? D3D12_HEAP_TYPE_UPLOAD
            : D3D12_HEAP_TYPE_DEFAULT;

        // UPLOAD buffers must be created in GENERIC_READ so the CPU can
        // write them. DEFAULT buffers can be in COMMON at creation time.
        m_InitialState = (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
            ? D3D12_RESOURCE_STATE_GENERIC_READ
            : D3D12_RESOURCE_STATE_COMMON;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type                 = m_HeapType;
        heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask     = 1;
        heapProps.VisibleNodeMask      = 1;

        const UINT64 alignedSize = AlignUp(static_cast<UINT64>(desc.Size), kBufferAlignment);

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment          = 0;  // 0 = use default alignment
        resourceDesc.Width              = alignedSize;
        resourceDesc.Height             = 1;
        resourceDesc.DepthOrArraySize   = 1;
        resourceDesc.MipLevels          = 1;
        resourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count   = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags              = TranslateResourceFlags(desc.Usage);

        HRESULT hr = device.GetD3D12Device()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            m_InitialState,
            nullptr,                    // no optimized clear value (buffers don't have one)
            IID_PPV_ARGS(&m_Resource));

        if (FAILED(hr))
        {
            XENGINE_LOG_ERROR(
                std::string("D3D12Buffer::D3D12Buffer: CreateCommittedResource failed ")
                + "(HRESULT=0x" + std::to_string(static_cast<unsigned long>(hr))
                + ", Size=" + std::to_string(desc.Size) + ")");
            m_Resource = nullptr;
        }
    }

    D3D12Buffer::~D3D12Buffer()
    {
        // WRL ComPtr releases the ID3D12Resource on destruction; no
        // explicit reset needed unless the resource is still in flight
        // on the GPU, which is handled at higher levels (frame fencing).
    }

    void D3D12Buffer::Update(u64 offset, const void* data, u64 size)
    {
        if (m_Resource == nullptr || data == nullptr || size == 0)
        {
            return;
        }
        if (m_HeapType != D3D12_HEAP_TYPE_UPLOAD)
        {
            // M4 only supports in-place update on UPLOAD heaps. The
            // DEFAULT-heap path requires a staging buffer + queue copy
            // and is provided by M11 RHIUploadManager.
            XENGINE_LOG_WARN(
                "D3D12Buffer::Update: ignored (DEFAULT-heap update requires "
                "M11 RHIUploadManager).");
            return;
        }
        if (offset + size > m_Size)
        {
            XENGINE_LOG_WARN(
                "D3D12Buffer::Update: write past end of buffer "
                "(offset + size > Size).");
            return;
        }

        // UPLOAD heap: write directly. D3D12_RANGE{0,0} tells the runtime
        // we're not reading from the mapped region (write-only), which
        // lets it skip an implicit readback barrier.
        void* mapped = nullptr;
        HRESULT hr = m_Resource->Map(0, nullptr, &mapped);
        if (FAILED(hr))
        {
            XENGINE_LOG_ERROR(
                std::string("D3D12Buffer::Update: Map failed (HRESULT=0x")
                + std::to_string(static_cast<unsigned long>(hr)) + ")");
            return;
        }
        std::memcpy(static_cast<char*>(mapped) + offset, data, static_cast<size_t>(size));
        m_Resource->Unmap(0, nullptr);
    }

    void* D3D12Buffer::Map()
    {
        if (m_Resource == nullptr || m_HeapType != D3D12_HEAP_TYPE_UPLOAD)
        {
            return nullptr;
        }
        void* mapped = nullptr;
        HRESULT hr = m_Resource->Map(0, nullptr, &mapped);
        if (FAILED(hr))
        {
            XENGINE_LOG_ERROR(
                std::string("D3D12Buffer::Map: Map failed (HRESULT=0x")
                + std::to_string(static_cast<unsigned long>(hr)) + ")");
            return nullptr;
        }
        return mapped;
    }

    void D3D12Buffer::Unmap()
    {
        if (m_Resource == nullptr || m_HeapType != D3D12_HEAP_TYPE_UPLOAD)
        {
            return;
        }
        m_Resource->Unmap(0, nullptr);
    }
}
