// D3D12Device — concrete RHIDevice for the D3D12 backend.
//
// Wraps an ID3D12Device2. Owns no queues in the skeleton phase — queue
// creation is deferred to M6 alongside command-list support.
//
// Skeleton scope (M0-M3):
//   * WaitIdle — no-op (no commands submitted yet; nothing to flush).
//   * GetBackend / GetCapabilities / GetMaxFramesInFlight — populated from
//     D3D12 feature-level query.
//   * GetQueue — returns nullptr. M6 wires up ID3D12CommandQueue.
//   * CreateBuffer/Texture/Sampler/Fence/Semaphore/CommandList Impl — all
//     return nullptr (M4/M5/M6 work).

#pragma once

#include <XEngine/RHI/RHIDevice.h>

#include <wrl/client.h>
#include <d3d12.h>

#include <memory>

namespace XEngine
{
    class D3D12Adapter;

    class D3D12Device : public RHIDevice
    {
    public:
        // Used by XEngine::CheckedCast<T> to reject cross-backend casts.
        static constexpr RHIBackend ExpectedBackend = RHIBackend::D3D12;

        // Backend factory. Returns nullptr when D3D12CreateDevice fails
        // (e.g. on an adapter that doesn't support the minimum feature
        // level). Constructed via private ctor — only this factory may
        // produce one.
        static std::unique_ptr<D3D12Device> Create(
            D3D12Adapter& adapter,
            const RHIDeviceDesc& desc);

        ~D3D12Device() override;

        // RHIDevice interface
        void WaitIdle() override;
        RHIBackend GetBackend() const noexcept override { return RHIBackend::D3D12; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return m_MaxFramesInFlight; }
        RHIFeature GetEnabledFeatures() const noexcept override { return m_EnabledFeatures; }
        RHIQueue* GetQueue(RHIQueueType type) const override;

        RHIBuffer* CreateBufferImpl(const RHIBufferDesc&) override { return nullptr; }
        RHITexture* CreateTextureImpl(const RHITextureDesc&) override { return nullptr; }
        RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc&) override { return nullptr; }
        RHISampler* CreateSamplerImpl(const RHISamplerDesc&) override { return nullptr; }
        RHIFence* CreateFenceImpl(const RHIFenceDesc&) override { return nullptr; }
        RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc&) override { return nullptr; }
        RHICommandList* CreateCommandListImpl(const RHICommandListDesc&) override { return nullptr; }

        // D3D12-specific accessor.
        ID3D12Device2* GetD3D12Device() const noexcept { return m_Device.Get(); }

    private:
        void PopulateCapabilities();
        void PopulateMaxFramesInFlight(const RHIDeviceDesc& desc);

        D3D12Device() = default;

        Microsoft::WRL::ComPtr<ID3D12Device2> m_Device;
        RHICapabilities m_Caps;
        u32 m_MaxFramesInFlight = 2;

        // Skeleton: no queue creation yet, so no features are enabled.
        // M3 will populate by walking the request through
        // ID3D12Device::CheckFeatureSupport. Always a subset of
        // adapter.GetSupportedFeatures().
        RHIFeature m_EnabledFeatures = RHIFeature::None;

        D3D_FEATURE_LEVEL m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
    };
}