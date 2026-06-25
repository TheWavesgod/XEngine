#pragma once

#include <XEngine/RHI/Resources/RHITextureView.h>

#include <volk.h>

namespace XEngine
{
    class VulkanDevice;

    class VulkanTextureView final : public RHITextureView
    {
    public:
        VulkanTextureView() = default;
        VulkanTextureView(VulkanDevice& device, const RHITextureViewDesc& desc);
        ~VulkanTextureView() override;

        VulkanTextureView(const VulkanTextureView&) = delete;
        VulkanTextureView& operator=(const VulkanTextureView&) = delete;

        bool IsValid() const;

        const RHITextureViewDesc& GetDesc() const override;
        
        VkImageView GetHandle() const;
        void* GetNativeView(RHIBackend backend) const override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        RHITextureViewDesc m_Desc {};  
    };
}