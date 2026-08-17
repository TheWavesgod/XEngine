// D3D12Buffer — concrete RHIBuffer for the D3D12 backend.
//
// M4: wraps an ID3D12Resource. Uses CreateCommittedResource (no
// sub-allocation / heap arena at M4) with the heap type and initial
// state selected from the requested RHIBufferUsage flags:
//
//   Uniform / TransferSrc -> D3D12_HEAP_TYPE_UPLOAD  (host-visible)
//   everything else        -> D3D12_HEAP_TYPE_DEFAULT (device-local)
//
// UPLOAD buffers are usable from Map / Unmap / Update in-place. DEFAULT
// buffers are read-only from the CPU side; their Update() path is
// deferred to M11 RHIUploadManager which will route through a
// UPLOAD-staging + CopyBufferRegion.
//
// Buffer sizes are rounded up to D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
// (64 KiB) per D3D12 alignment requirements, but the original
// RHIBufferDesc::Size is preserved verbatim for GetSize().

#pragma once

#include <XEngine/RHI/RHIBuffer.h>
#include <XEngine/RHI/RHIEnums.h>

#include <wrl/client.h>
#include <d3d12.h>

#include <memory>

namespace XEngine
{
    class D3D12Device;

    class D3D12Buffer : public RHIBuffer
    {
    public:
        // Used by XEngine::CheckedCast<T> to reject cross-backend casts.
        static constexpr RHIBackend ExpectedBackend = RHIBackend::D3D12;

        // Backend factory. Returns nullptr on CreateCommittedResource failure.
        // Constructed via private ctor — only this factory may produce one.
        static std::unique_ptr<D3D12Buffer> Create(
            D3D12Device& device,
            const RHIBufferDesc& desc);

        // Public ctor — used by the static factory above.
        D3D12Buffer(D3D12Device& device, const RHIBufferDesc& desc);

        ~D3D12Buffer() override;

        // RHIBuffer interface
        u64            GetSize()  const noexcept override { return m_Size; }
        RHIBufferUsage GetUsage() const noexcept override { return m_Usage; }
        void           Update(u64 offset, const void* data, u64 size) override;
        void*          Map() override;
        void           Unmap() override;

        // D3D12-specific accessors
        ID3D12Resource* GetD3D12Resource() const noexcept { return m_Resource.Get(); }
        D3D12_HEAP_TYPE GetHeapType()      const noexcept { return m_HeapType; }
        bool            IsHostVisible()    const noexcept { return m_HeapType == D3D12_HEAP_TYPE_UPLOAD; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        D3D12_HEAP_TYPE        m_HeapType     = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_STATES  m_InitialState = D3D12_RESOURCE_STATE_COMMON;
        u64                    m_Size         = 0;  // original (pre-alignment)
        RHIBufferUsage         m_Usage        = RHIBufferUsage::None;
    };
}
