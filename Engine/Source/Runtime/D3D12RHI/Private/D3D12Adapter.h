// D3D12Adapter — concrete RHIAdapter for the D3D12 backend.
//
// Wraps an IDXGIAdapter1. Exposes static info (vendor, name, type, memory)
// and forwards device creation to the owning RHIInstance.
//
// Skeleton scope (M0-M3):
//   * GetInfo() — fully populated from DXGI_ADAPTER_DESC1.
//   * GetSupportedFeatures() — returns RHIFeature::None. M3+ will fill in
//     by querying ID3D12Device::CheckFeatureSupport after a device exists.
//   * SupportsRequiredCapabilities() — M3 stub returns true.

#pragma once

#include <XEngine/RHI/RHIAdapter.h>
#include <XEngine/RHI/RHIEnums.h>

#include <wrl/client.h>
#include <dxgi1_4.h>

#include <string>

namespace XEngine
{
    class D3D12Instance;

    class D3D12Adapter : public RHIAdapter
    {
    public:
        D3D12Adapter(D3D12Instance& instance,
                     Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter);
        ~D3D12Adapter() override;

        // RHIAdapter interface
        RHIAdapterInfo GetInfo() const override;
        RHIFeature GetSupportedFeatures() const noexcept override { return m_SupportedFeatures; }
        bool SupportsRequiredCapabilities(const RHICapabilities& required) const override;

        // D3D12-specific accessors.
        IDXGIAdapter1* GetDXGIAdapter() const noexcept { return m_Adapter.Get(); }
        D3D12Instance& GetD3D12Instance() const noexcept { return m_Instance; }

    private:
        void QueryDesc();

        D3D12Instance& m_Instance;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> m_Adapter;

        // Stable storage for std::string_view fields in m_Info. DXGI only
        // exposes Description (the human-readable adapter name) — Vendor
        // is implied by VendorId.
        std::string m_DeviceName;
        std::string m_ApiInfo;
        RHIAdapterInfo m_Info;

        RHIFeature m_SupportedFeatures = RHIFeature::None;
    };
}