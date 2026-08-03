// RHIInstance — base class implementation file.
//
// M2 contains only the static factory stub and the adapter scoring
// algorithm. The base-class virtual services (EnumerateAdapters,
// CreateDevice) are pure virtual — concrete backends (VulkanRHI / D3D12RHI
// / MetalRHI) implement them in later milestones.

#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIAdapter.h>  // RHIAdapterInfo + RHIAdapter needed by ScoreAdapter + RequestAdapter bodies

#include <utility>

namespace XEngine
{
    // M2 stub — no backend target exists yet. M3 dispatches to the
    // appropriate backend factory based on RHIInstanceDesc + platform.
    std::unique_ptr<RHIInstance> RHIInstance::Create(const RHIInstanceDesc& desc)
    {
        (void)desc;
        return nullptr;
    }

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
