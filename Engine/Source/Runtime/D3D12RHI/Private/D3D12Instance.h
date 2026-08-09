// D3D12Instance — concrete RHIInstance for the D3D12 backend.
//
// Owns the IDXGIFactory4 used to enumerate adapters. Adapter enumeration
// and device creation are the two backend hooks we override; everything
// else (RequestAdapter default implementation, single-device rule, feature
// negotiation) is handled by the RHIInstance base class and inherited
// unchanged.
//
// Skeleton scope (M0-M3 backend):
//   * EnumerateAdapters — wraps IDXGIFactory4::EnumAdapters1, returns one
//     D3D12Adapter per IDXGIAdapter1.
//   * CreateDeviceImpl — constructs a D3D12Device over the chosen adapter.
//
// Phase 2+ (M4+) will add queue creation, command-list support, and
// swapchain creation.

#pragma once

#include <XEngine/RHI/RHIInstance.h>

// D3D12 / DXGI / WRL headers are PRIVATE to this target. Public consumers
// of XEngineD3D12RHI see only XEngine/D3D12RHI/Backend.h.
//
// The target is gated to WIN32 by Engine/CMakeLists.txt, so the Windows
// path is always taken when this header is in scope. The #ifndef guard
// below is defensive — keeps the header parseable if a stray include
// ever happens on a non-Windows toolchain.
#ifndef _WIN32
    // Forward-declare the type just enough to keep the class well-formed
    // for IDE / parser purposes on non-Windows hosts.
    struct IDXGIFactory4;
#endif

#include <memory>
#include <vector>

#ifdef _WIN32
    #include <wrl/client.h>
    #include <dxgi1_4.h>
#endif

namespace XEngine
{
    class D3D12Adapter;
    class D3D12Device;

    class D3D12Instance : public RHIInstance
    {
    public:
        // Used by XEngine::CheckedCast<T> to reject cross-backend casts.
        static constexpr RHIBackend ExpectedBackend = RHIBackend::D3D12;

        // Factory: creates an IDXGIFactory4 via CreateDXGIFactory1 and
        // constructs a D3D12Instance. Returns nullptr if the factory
        // creation fails (typically no DXGI runtime, e.g. on pre-Win7 or
        // headless containers without a display subsystem).
        static std::unique_ptr<D3D12Instance> Create(const RHIInstanceDesc& desc);

        D3D12Instance(Microsoft::WRL::ComPtr<IDXGIFactory4> factory,
                      const RHIInstanceDesc& desc);
        ~D3D12Instance() override;

        // RHIInstance interface — see RHIInstance.h for contract.
        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override;

        // Backend hook — see RHIInstance::CreateDevice for the contract.
        std::unique_ptr<RHIDevice> CreateDeviceImpl(
            RHIAdapter& adapter,
            const RHIDeviceDesc& desc) override;

        // D3D12-specific accessor. Returns nullptr on non-Windows hosts.
        IDXGIFactory4* GetDXGIFactory() const noexcept;

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
    };
}