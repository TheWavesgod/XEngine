// D3D12Instance — implementation.
//
// Note: this translation unit is compiled only when XENGINE_BUILD_D3D12_RHI
// is ON on a WIN32 platform (see Engine/CMakeLists.txt and
// D3D12RHI/CMakeLists.txt). The contents are therefore unconditionally
// Windows-only.

#include "D3D12Instance.h"
#include "D3D12Adapter.h"
#include "D3D12Device.h"

#include <XEngine/RHI/RHICast.h>
#include <XEngine/Logging/Log.h>

#include <wrl/client.h>
#include <dxgi1_4.h>
#include <string>

namespace XEngine
{
    namespace D3D12RHI
    {
        // Free-function factory declared in Backend.h. Forwards to the
        // static factory on the concrete class so RHIRuntime's factory
        // pointer points to a single entry point.
        std::unique_ptr<RHIInstance> CreateInstance(const RHIInstanceDesc& desc)
        {
            return D3D12Instance::Create(desc);
        }
    }

    std::unique_ptr<D3D12Instance> D3D12Instance::Create(const RHIInstanceDesc& desc)
    {
        // CreateDXGIFactory1 is exported by dxgi.lib; on platforms without
        // a display subsystem the call returns E_FAIL. We don't try WARP
        // here — the App is expected to ask for an explicit WARP device
        // through a future RHIDeviceDesc flag. For the skeleton, a failed
        // factory creation means "no D3D12 available" → App gets nullptr.
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
        if (FAILED(hr))
        {
            XENGINE_LOG_WARN(
                std::string("D3D12Instance::Create: CreateDXGIFactory1 failed ")
                + "(HRESULT=0x" + std::to_string(static_cast<unsigned long>(hr))
                + "). D3D12 backend unavailable on this machine.");
            return nullptr;
        }

        return std::unique_ptr<D3D12Instance>(new D3D12Instance(factory, desc));
    }

    D3D12Instance::D3D12Instance(Microsoft::WRL::ComPtr<IDXGIFactory4> factory,
                                 const RHIInstanceDesc& desc)
        : RHIInstance(desc, RHIBackend::D3D12)
        , m_Factory(std::move(factory))
    {
    }

    D3D12Instance::~D3D12Instance()
    {
        // The base class owns m_Device (a unique_ptr<RHIDevice>). Reset it
        // before tearing down the DXGI factory so the D3D12 device is
        // released before its adapter / factory references.
        m_Device.reset();
        m_Factory.Reset();
    }

    std::vector<std::unique_ptr<RHIAdapter>> D3D12Instance::EnumerateAdapters()
    {
        std::vector<std::unique_ptr<RHIAdapter>> adapters;

        // IDXGIFactory4::EnumAdapters1 returns IDXGIAdapter1 directly with
        // the DXGI_ADAPTER_DESC1 layout (DedicatedVideoMemory,
        // DedicatedSystemMemory, SharedSystemMemory fields). This is the
        // recommended path on Windows 10+ where IDXGIFactory4 is always
        // available.
        for (UINT i = 0; ; ++i)
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
            HRESULT hr = m_Factory->EnumAdapters1(i, &adapter);
            if (hr == DXGI_ERROR_NOT_FOUND)
            {
                break;  // End of enumeration.
            }
            if (FAILED(hr))
            {
                XENGINE_LOG_WARN(
                    std::string("D3D12Instance::EnumerateAdapters: EnumAdapters1[")
                    + std::to_string(i) + "] failed (HRESULT=0x"
                    + std::to_string(static_cast<unsigned long>(hr)) + ").");
                continue;
            }

            adapters.push_back(std::make_unique<D3D12Adapter>(*this, adapter));
        }
        return adapters;
    }

    std::unique_ptr<RHIDevice> D3D12Instance::CreateDeviceImpl(
        RHIAdapter& adapter,
        const RHIDeviceDesc& desc)
    {
        auto& d3d12Adapter = XEngine::CheckedCast<D3D12Adapter>(adapter);
        return D3D12Device::Create(d3d12Adapter, desc);
    }
}