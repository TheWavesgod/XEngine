#pragma once

#include <XEngine/RHI/Resources/RHIPipeline.h>

#include <volk.h>

namespace XEngine
{
    class VulkanDevice;

    class VulkanPipeline final : public RHIPipeline
    {
    public:
        VulkanPipeline(VulkanDevice& device, const RHIGraphicsPipelineDesc& desc);
        ~VulkanPipeline() override;

        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        bool IsValid() const;

        VkPipeline GetHandle() const;
        VkPipelineLayout GetLayout() const;
        VkShaderStageFlags GetPushConstantStages() const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkShaderStageFlags m_PushConstantStages = 0;
    };
}
