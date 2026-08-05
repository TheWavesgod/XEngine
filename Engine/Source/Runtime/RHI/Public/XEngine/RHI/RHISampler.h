// RHISampler — GPU sampler.
//
// M5 surface (query plus captured descriptor):
//   * GetDesc() — returns the cached RHISamplerDesc used at creation
//   * No Map / Update — samplers are pure GPU state
//
// M5 audit items addressed:
//   Vulkan 3.1 — RHISamplerDesc captures all 7 fields (CompareEnable,
//                CompareOp, BorderColor, MinLod, MaxLod, LodBias +
//                AddressMode/FilterMode from the original 3 fields)

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIDescriptors.h>

namespace XEngine
{
    // RHISampler — backend sampler (VkSampler, ID3D12SamplerState, id<MTLSamplerState>).
    class RHISampler : public RHIObject
    {
    public:
        virtual ~RHISampler() override = default;

        // Returns the descriptor used to create this sampler. Backends
        // typically capture the RHISamplerDesc at creation time.
        virtual RHISamplerDesc GetDesc() const noexcept = 0;

        // Non-copyable / non-movable: backend samplers wrap native handles.
        RHISampler(const RHISampler&) = delete;
        RHISampler& operator=(const RHISampler&) = delete;
        RHISampler(RHISampler&&) = delete;
        RHISampler& operator=(RHISampler&&) = delete;

    protected:
        explicit RHISampler(RHIDevice& owner) noexcept
            : RHIObject(owner)
        {
        }

        RHISampler(RHIDevice& owner, RHIBackend backend) noexcept
            : RHIObject(owner, backend)
        {
        }
    };
}
