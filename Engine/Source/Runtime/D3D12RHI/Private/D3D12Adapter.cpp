// D3D12Adapter — implementation.

#include "D3D12Adapter.h"
#include "D3D12Instance.h"

#include <XEngine/RHI/RHIDevice.h>  // for RHICapabilities

#include <wrl/client.h>
#include <dxgi1_4.h>

#include <string>

namespace
{
    // Convert a DXGI WCHAR string to UTF-8 std::string. Returns empty
    // string on allocation failure or empty source. Used for adapter
    // names which are wide-character DXGI_ADAPTER_DESC1::Description.
    std::string WideToUtf8(const wchar_t* w)
    {
        if (w == nullptr || *w == L'\0')
        {
            return {};
        }

        // First query the required buffer size.
        const int needed = WideCharToMultiByte(
            CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (needed <= 0)
        {
            return {};
        }

        std::string out(static_cast<size_t>(needed - 1), '\0');  // exclude null terminator
        const int written = WideCharToMultiByte(
            CP_UTF8, 0, w, -1, out.data(), needed, nullptr, nullptr);
        if (written <= 0)
        {
            return {};
        }
        return out;
    }

    XEngine::RHIAdapterType MapAdapterType(DXGI_ADAPTER_DESC1 const& desc)
    {
        // DXGI doesn't expose an explicit integrated / discrete bit — we
        // infer from vendor id range and DedicatedVideoMemory size. This
        // matches the convention in VulkanRHI where similar inference is
        // done from VkPhysicalDeviceProperties.
        constexpr UINT kIntelVendorId     = 0x8086;
        constexpr UINT kAmdVendorId       = 0x1002;
        constexpr UINT kNvidiaVendorId    = 0x10DE;
        constexpr UINT kMicrosoftVendorId = 0x1414;  // WARP / Basic Render

        if (desc.VendorId == kMicrosoftVendorId)
        {
            return XEngine::RHIAdapterType::CPU;  // WARP is software-rasterized.
        }
        // Most discrete GPUs report DedicatedVideoMemory >= 1 GiB. Anything
        // below that on a recognized vendor is treated as integrated. This
        // is the same heuristic DXGI samples and the Steam hardware survey
        // apply.
        constexpr UINT64 kDiscreteThresholdBytes = UINT64(1) << 30;
        const bool isDiscrete = desc.DedicatedVideoMemory >= kDiscreteThresholdBytes;
        if (isDiscrete)
        {
            return XEngine::RHIAdapterType::Discrete;
        }
        if (desc.VendorId == kIntelVendorId
            || desc.VendorId == kAmdVendorId
            || desc.VendorId == kNvidiaVendorId)
        {
            return XEngine::RHIAdapterType::Integrated;
        }
        return XEngine::RHIAdapterType::Unknown;
    }
}

namespace XEngine
{
    D3D12Adapter::D3D12Adapter(D3D12Instance& instance,
                               Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter)
        : RHIAdapter(instance, RHIBackend::D3D12)
        , m_Instance(instance)
        , m_Adapter(std::move(adapter))
    {
        QueryDesc();
    }

    D3D12Adapter::~D3D12Adapter() = default;

    void D3D12Adapter::QueryDesc()
    {
        DXGI_ADAPTER_DESC1 desc{};
        HRESULT hr = m_Adapter->GetDesc1(&desc);
        if (FAILED(hr))
        {
            // GetDesc1 rarely fails, but if it does we leave m_Info as
            // zeros — the adapter will still be enumerable, just with no
            // human-readable name.
            return;
        }

        // DXGI only exposes Description (the human-readable adapter name).
        // Driver version requires IUnknown::QueryInterface into the actual
        // IDXGIAdapter1's underlying IDXGIAdapter (legacy GetDesc) which
        // reports DriverVersion; for the skeleton we leave it empty.
        m_DeviceName = WideToUtf8(desc.Description);
        m_ApiInfo    = "DirectX 12";

        m_Info.VendorName           = m_DeviceName;
        m_Info.AdapterName          = m_DeviceName;
        m_Info.DriverVersion        = {};
        m_Info.APIInfo              = m_ApiInfo;
        m_Info.VendorID             = desc.VendorId;
        m_Info.DeviceID             = desc.DeviceId;
        m_Info.Type                 = MapAdapterType(desc);
        m_Info.DedicatedMemoryBytes = desc.DedicatedVideoMemory;
        m_Info.SharedMemoryBytes    = desc.SharedSystemMemory;
    }

    RHIAdapterInfo D3D12Adapter::GetInfo() const
    {
        return m_Info;
    }

    bool D3D12Adapter::SupportsRequiredCapabilities(const RHICapabilities& required) const
    {
        // Skeleton: nothing has been detected yet, so the answer is
        // unconditional accept. Real capability checks land in M3 once
        // D3D12Device::CheckFeatureSupport is wired in.
        (void)required;
        return true;
    }
}