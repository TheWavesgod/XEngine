#pragma once

#include <XEngine/RHI/Resources/RHIShader.h>

#include <volk.h>

#include <string>

namespace XEngine
{
    class VulkanShader final : public RHIShader
    {
    public:
        VulkanShader(class VulkanDevice& device, const RHIShaderDesc& desc);
        ~VulkanShader();

        VulkanShader(const VulkanShader&) = delete;
        VulkanShader& operator=(const VulkanShader&) = delete;

        bool IsValid() const;

        ShaderStage GetStage() const override;
        ShaderTarget GetTarget() const override;

        VkShaderModule GetHandle() const;
        const std::string& GetEntryPoint() const;
        VkShaderStageFlagBits GetVulkanStage() const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
        ShaderStage m_Stage = ShaderStage::Unknown;
        ShaderTarget m_Target = ShaderTarget::Unknown;
        std::string m_EntryPoint;
    };
}
