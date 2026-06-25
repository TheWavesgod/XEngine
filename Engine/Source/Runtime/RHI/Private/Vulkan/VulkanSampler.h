#pragma once

#include <XEngine/RHI/Resources/RHISampler.h>

#include <volk.h>

namespace XEngine
{
    class VulkanSampler final : public RHISampler
    {
    public:
        VulkanSampler() = default;
        VulkanSampler(class VulkanDevice& device, const RHISamplerDesc& desc);
        ~VulkanSampler() override;

        VulkanSampler(const VulkanSampler&) = delete;
        VulkanSampler& operator=(const VulkanSampler&) = delete;

        bool IsValid() const;
        const RHISamplerDesc& GetDesc() const override;
        VkSampler GetHandle() const;
        void* GetNativeSampler(RHIBackend backend) const override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
        RHISamplerDesc m_Desc {};
    };
}
