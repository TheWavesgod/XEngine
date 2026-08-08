// RHIInstance — base class implementation file.
//
// M2 contains the adapter scoring algorithm and the NVI CreateDevice
// wrapper. The base-class virtual services (EnumerateAdapters,
// CreateDeviceImpl) are pure virtual — concrete backends (VulkanRHI /
// D3D12RHI / MetalRHI) implement them in later milestones.
//
// The static RHIInstance::Create() factory was removed in the M2 multi-
// backend rework (see plan §13 / M2). Backend dispatch is now in
// XEngine::RHIRuntime::CreateInstance (see <XEngine/RHI/RHIRuntime.h>).

#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIAdapter.h>  // RHIAdapterInfo + RHIAdapter needed by ScoreAdapter + RequestAdapter bodies
#include <XEngine/RHI/RHIValidation.h>

#include <utility>

namespace XEngine
{
    // Default RequestAdapter implementation: enumerate, score, pick best.
    // Backends can override if they have a more efficient backend-specific
    // path (e.g., fusing the score computation with vkEnumeratePhysicalDevices).
    std::unique_ptr<RHIAdapter> RHIInstance::RequestAdapter(
        RHIAdapterPreference preference,
        const RHICapabilities& required)
    {
        std::unique_ptr<RHIAdapter> best;
        u32 bestScore = 0;

        for (auto& adapter : EnumerateAdapters())
        {
            if (!adapter->SupportsRequiredCapabilities(required))
            {
                continue;
            }

            const u32 score = ScoreAdapter(adapter->GetInfo(), preference);
            if (score > bestScore)
            {
                best = std::move(adapter);
                bestScore = score;
            }
        }

        return best;
    }

    // NVI wrapper enforcing the single-device rule + RequiredFeatures
    // invariant. Backends never see this method; they override the impl.
    RHIDevice* RHIInstance::CreateDevice(RHIAdapter& adapter, const RHIDeviceDesc& desc)
    {
        // (1) Single-device rule.
        if (m_Device)
        {
            XENGINE_LOG_WARN("RHIInstance::CreateDevice: device already exists; returning nullptr.");
            return nullptr;
        }

        // (2) Descriptor validation.
        if (auto r = ValidateDeviceDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }

        // (3) RequiredFeatures ⊆ SupportedFeatures.
        const RHIFeature supported = adapter.GetSupportedFeatures();
        using U = std::underlying_type_t<RHIFeature>;
        const auto missingBits = static_cast<U>(desc.RequiredFeatures) & ~static_cast<U>(supported);
        if (missingBits != 0u)
        {
            XENGINE_LOG_ERROR("RHIInstance::CreateDevice: adapter does not support required features (missing bits).");
            return nullptr;
        }

        // (4) Delegate to backend.
        auto dev = CreateDeviceImpl(adapter, desc);
        if (!dev)
        {
            return nullptr;
        }

        // (5) Take ownership.
        m_Device = std::move(dev);
        return m_Device.get();
    }

    // Adapter scoring algorithm. Higher = better. 0 means unsuitable.
    //
    //   Automatic / HighPerformance: VRAM-weighted discrete GPU wins.
    //   LowPower:                     integrated > discrete > CPU.
    //   Explicit:                     0 — never picked by score; M11 opens an
    //                                 explicit-ID selection API.
    u32 RHIInstance::ScoreAdapter(const RHIAdapterInfo& info, RHIAdapterPreference preference)
    {
        // Reject unknown adapters outright.
        if (info.Type == RHIAdapterType::Unknown)
        {
            return 0;
        }

        switch (preference)
        {
            case RHIAdapterPreference::Automatic:
            case RHIAdapterPreference::HighPerformance:
            {
                u32 score = 0;
                switch (info.Type)
                {
                    case RHIAdapterType::Discrete:
                        // 100 base + 1 per 256 MB of dedicated VRAM, capped at 200.
                        score = 100 + static_cast<u32>(
                            info.DedicatedMemoryBytes / (256ull * 1024 * 1024));
                        if (score > 200) { score = 200; }
                        break;
                    case RHIAdapterType::Integrated:
                        score = 30;
                        break;
                    case RHIAdapterType::CPU:
                        score = 1;
                        break;
                    case RHIAdapterType::Unknown:
                        return 0;
                }
                return score;
            }

            case RHIAdapterPreference::LowPower:
            {
                // Higher = better. Integrated > Discrete > CPU.
                switch (info.Type)
                {
                    case RHIAdapterType::Integrated: return 100;
                    case RHIAdapterType::Discrete:   return 10;
                    case RHIAdapterType::CPU:        return 1;
                    case RHIAdapterType::Unknown:    return 0;
                }
                return 0;
            }

            case RHIAdapterPreference::Explicit:
                // Never picked by score; the M11 explicit-ID path bypasses scoring.
                return 0;
        }

        return 0;
    }
}
