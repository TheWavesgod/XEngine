// D3D12Device — implementation.

#include "D3D12Device.h"
#include "D3D12Adapter.h"

#include <XEngine/Logging/Log.h>

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <string>

namespace XEngine
{
    std::unique_ptr<D3D12Device> D3D12Device::Create(
        D3D12Adapter& adapter,
        const RHIDeviceDesc& desc)
    {
        // D3D12CreateDevice takes an adapter IUnknown. We pass the
        // IDXGIAdapter1 directly — D3D12 internally walks to the
        // underlying ID3D12Device through this interface.
        Microsoft::WRL::ComPtr<ID3D12Device2> device;

        // Minimum feature level: 11_0 keeps the device creation compatible
        // with WARP and older GPUs. Feature negotiation in M3 will let
        // Apps request a higher minimum (e.g. SM6 via 12_0 / 12_1 / 12_2)
        // through RHIDeviceDesc.
        constexpr D3D_FEATURE_LEVEL kMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        HRESULT hr = D3D12CreateDevice(
            adapter.GetDXGIAdapter(),
            kMinFeatureLevel,
            IID_PPV_ARGS(&device));

        if (FAILED(hr))
        {
            XENGINE_LOG_WARN(
                std::string("D3D12Device::Create: D3D12CreateDevice failed ")
                + "(HRESULT=0x" + std::to_string(static_cast<unsigned long>(hr))
                + "). Adapter does not support D3D12 at the minimum "
                + "feature level.");
            return nullptr;
        }

        // Friend access via std::make_unique — D3D12Device's ctor is
        // private so consumers must go through this factory.
        auto out = std::unique_ptr<D3D12Device>(new D3D12Device());
        out->m_Device = std::move(device);
        out->m_FeatureLevel = kMinFeatureLevel;
        out->PopulateMaxFramesInFlight(desc);
        out->PopulateCapabilities();
        return out;
    }

    D3D12Device::~D3D12Device()
    {
        // ID3D12Device2 releases its internal allocations via the WRL
        // ComPtr destructor; no explicit Flush is needed because no
        // command lists / queues were created in the skeleton phase.
    }

    void D3D12Device::PopulateMaxFramesInFlight(const RHIDeviceDesc& desc)
    {
        // Caller's preference, clamped to a sane range. D3D12 itself
        // does not impose an upper bound — the caller's MaxFramesInFlight
        // determines allocator / descriptor pool sizing.
        if (desc.MaxFramesInFlight == 0)
        {
            m_MaxFramesInFlight = 2;
        }
        else if (desc.MaxFramesInFlight > 4)
        {
            m_MaxFramesInFlight = 4;
        }
        else
        {
            m_MaxFramesInFlight = desc.MaxFramesInFlight;
        }
    }

    void D3D12Device::PopulateCapabilities()
    {
        // Skeleton: report conservative defaults that mirror what
        // D3D12 guarantees at feature level 11_0. Real values from
        // ID3D12Device::CheckFeatureSupport land in M3+.
        m_Caps.MaxTextureSize2D            = 16384;
        m_Caps.MaxBufferSize               = UINT64(1) << 30;  // 1 GiB
        m_Caps.MaxSamplerAnisotropy        = 16;
        m_Caps.MaxSampleCount              = 8;
        m_Caps.MaxViewports                = 1;       // M3 may bump
        m_Caps.MaxColorAttachments         = 8;       // D3D12 fixed at 8
        m_Caps.MaxFramesInFlight           = m_MaxFramesInFlight;
        m_Caps.MaxComputeWorkGroupInvocations = 1024;  // M3 verified via CheckFeatureSupport
        m_Caps.MaxComputeSharedMemorySize     = 32 * 1024;

        // Alignment requirements come from ID3D12Device::GetCopyableFootprints;
        // skeleton uses safe defaults.
        m_Caps.MinUniformBufferOffsetAlignment = 256;
        m_Caps.MinStorageBufferOffsetAlignment = 16;
        m_Caps.MinTexelBufferOffsetAlignment    = 16;

        // D3D12 always-on capabilities (true at FL_11_0):
        m_Caps.SupportsDepthClip            = true;
        m_Caps.SupportsDepthBiasClamp       = true;
        m_Caps.SupportsWideLines            = true;
        m_Caps.SupportsLargePoints          = true;

        // M3 will populate the rest via CheckFeatureSupport:
        //   TimelineSemaphore (FL_12_0+),
        //   PushDescriptor   (not applicable to D3D12 — false),
        //   Bindless         (resource binding tier 2+),
        //   BufferDeviceAddress (FL_12_0+),
        //   RayTracing       (FL_12_0 + DXR),
        //   GeometryShader   (FL_11_0+, true),
        //   Tessellation     (FL_11_0+, true).
        m_Caps.SupportsTimelineSemaphore    = (m_FeatureLevel >= D3D_FEATURE_LEVEL_12_0);
        m_Caps.SupportsPushDescriptor       = false;
        m_Caps.SupportsBindless             = false;
        m_Caps.SupportsBufferDeviceAddress  = (m_FeatureLevel >= D3D_FEATURE_LEVEL_12_0);
        m_Caps.SupportsRayTracing           = false;
        m_Caps.SupportsGeometryShader       = true;
        m_Caps.SupportsTessellationShader   = true;
    }

    void D3D12Device::WaitIdle()
    {
        // Skeleton: no queues / command lists exist yet, so there is
        // nothing to wait on. M6 will replace this with a transient
        // fence on the graphics queue, Signaled → Event-wait pattern.
    }

    RHIQueue* D3D12Device::GetQueue(RHIQueueType /*type*/) const
    {
        // M6 will return a D3D12Queue wrapper around ID3D12CommandQueue.
        return nullptr;
    }
}