#include "VulkanShader.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <cstring>
#include <string>
#include <vector>

namespace XEngine
{
    namespace
    {
        VkShaderStageFlagBits ToVulkanShaderStage(ShaderStage stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderStage::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderStage::Compute:
                return VK_SHADER_STAGE_COMPUTE_BIT;
            default:
                return static_cast<VkShaderStageFlagBits>(0);
            }
        }
    }

    VulkanShader::VulkanShader(VulkanDevice& device, const RHIShaderDesc& desc)
        : RHIShader(device) 
        , m_Device(device.GetHandle())
        , m_Stage(desc.Stage)
        , m_Target(desc.Target)
        , m_EntryPoint(desc.EntryPoint)
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan shader module without a valid device");
            return;
        }

        if (desc.Target != ShaderTarget::VulkanSPIRV || desc.Format != ShaderCodeFormat::Binary)
        {
            XENGINE_LOG_ERROR("VulkanShader requires Vulkan SPIR-V binary shader code");
            return;
        }

        if (desc.Code == nullptr || desc.CodeSize == 0 || (desc.CodeSize % sizeof(u32)) != 0)
        {
            XENGINE_LOG_ERROR("VulkanShader received invalid SPIR-V bytecode");
            return;
        }

        if (ToVulkanShaderStage(desc.Stage) == 0)
        {
            XENGINE_LOG_ERROR("VulkanShader received unsupported shader stage");
            return;
        }

        std::string message = "Creating Vulkan shader module";
        if (desc.DebugName != nullptr)
        {
            message += ": ";
            message += desc.DebugName;
        }
        XENGINE_LOG_INFO(message);

        std::vector<u32> spirvWords(desc.CodeSize / sizeof(u32));
        std::memcpy(spirvWords.data(), desc.Code, desc.CodeSize);

        VkShaderModuleCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = desc.CodeSize;
        createInfo.pCode = spirvWords.data();

        VkResult result = vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_ShaderModule);
        if (result != VK_SUCCESS)
        {
            std::string error = "Failed to create Vulkan shader module: ";
            error += VulkanResultToString(result);
            XENGINE_LOG_ERROR(error);
            return;
        }

        XENGINE_LOG_INFO("Vulkan shader module created");
    }

    VulkanShader::~VulkanShader()
    {
        if (m_Device != VK_NULL_HANDLE && m_ShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
            m_ShaderModule = VK_NULL_HANDLE;
        }
    }

    bool VulkanShader::IsValid() const
    {
        return m_ShaderModule != VK_NULL_HANDLE;
    }

    ShaderStage VulkanShader::GetStage() const
    {
        return m_Stage;
    }

    ShaderTarget VulkanShader::GetTarget() const
    {
        return m_Target;
    }

    VkShaderModule VulkanShader::GetHandle() const
    {
        return m_ShaderModule;
    }

    const std::string& VulkanShader::GetEntryPoint() const
    {
        return m_EntryPoint;
    }

    VkShaderStageFlagBits VulkanShader::GetVulkanStage() const
    {
        return ToVulkanShaderStage(m_Stage);
    }
}
