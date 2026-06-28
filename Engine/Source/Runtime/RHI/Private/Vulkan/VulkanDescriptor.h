#pragma once

#include <XEngine/RHI/Resources/RHIBindGroup.h>

#include <volk.h>

namespace XEngine
{
    class VulkanDevice;

    class VulkanBindGroupLayout final : public RHIBindGroupLayout
    {
    public:
        explicit VulkanBindGroupLayout(VulkanDevice& device);
        ~VulkanBindGroupLayout() override;

        bool Create(VulkanDevice& device, const RHIBindGroupLayoutDesc& desc);
        void Destroy();

        VkDescriptorSetLayout GetHandle() const;
        const RHIBindGroupLayoutDesc& GetDesc() const override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
        RHIBindGroupLayoutDesc m_Desc {};
    };

    class VulkanBindGroup final : public RHIBindGroup
    {
    public:
        explicit VulkanBindGroup(VulkanDevice& device);
        ~VulkanBindGroup() override;

        bool Create(
            VulkanDevice& device,
            VkDescriptorPool descriptorPool,
            const RHIBindGroupDesc& desc);

        void Destroy();

        VkDescriptorSet GetHandle() const;
        const RHIBindGroupDesc& GetDesc() const override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_Set = VK_NULL_HANDLE;
        RHIBindGroupDesc m_Desc {};
    };
}
