#pragma once

#include <XEngine/RHI/Resources/RHITexture.h>
#include <volk.h>
#include <vk_mem_alloc.h>

namespace XEngine
{
    class RHITextureView;
    class VulkanTextureView;

    class VulkanTexture final : public RHITexture
    {
    public:
        VulkanTexture() = default;
        VulkanTexture(class VulkanDevice& device, VmaAllocator allocator, const RHITextureDesc& desc);
        ~VulkanTexture() override;

        VulkanTexture(const VulkanTexture&) = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;

        bool IsValid() const;
        const RHITextureDesc& GetDesc() const override;

        VkImage GetImage() const;
        RHITextureView* GetDefaultView() const override;

        void* GetNativeDefaultView(RHIBackend backend) const override;
        VkImageLayout* GetLayoutPtr();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkImage m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_AllocationInfo {};
        VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        RHITextureDesc m_Desc {};

        mutable std::shared_ptr<RHITextureView> m_DefaultView;
    };
}
