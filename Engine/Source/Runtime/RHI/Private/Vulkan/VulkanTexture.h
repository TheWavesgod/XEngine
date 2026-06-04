#pragma once

#include <XEngine/RHI/Resources/RHITexture.h>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace XEngine
{
    class VulkanTexture final : public RHITexture
    {
    public:
        VulkanTexture() = default;
        VulkanTexture(VkDevice device, VmaAllocator allocator, const RHITextureDesc& desc);
        ~VulkanTexture() override;

        VulkanTexture(const VulkanTexture&) = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;

        bool IsValid() const;
        u32 GetWidth() const override;
        u32 GetHeight() const override;
        RHIFormat GetFormat() const override;

        VkImage GetImage() const;
        VkImageView GetImageView() const;
        VkImageLayout* GetLayoutPtr();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkImage m_Image = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_AllocationInfo {};
        VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        u32 m_Width = 0;
        u32 m_Height = 0;
        RHIFormat m_Format = RHIFormat::Undefined;
    };
}
