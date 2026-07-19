#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanCheckedCast.h"
#include "VulkanDescriptor.h"
#include "VulkanShader.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <array>
#include <string>
#include <vector>

namespace XEngine
{
    VulkanPipeline::VulkanPipeline(VulkanDevice& device, const RHIGraphicsPipelineDesc& desc)
        : RHIPipeline(device) 
        , m_Device(device.GetHandle())
        , m_PushConstantStages(ToVulkanShaderStageFlags(desc.PushConstantStages))
    {
        XENGINE_LOG_INFO("Creating Vulkan graphics pipeline");

        if (m_Device == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan pipeline without a valid device");
            return;
        }

        // Guard against oversized push-constant blocks. Vulkan guarantees only
        // 128 bytes for maxPushConstantsSize; some drivers (Intel Iris Xe Gen
        // 11/12 included) crash inside vkCreatePipelineLayout instead of
        // returning an error when a range exceeds the device's limit, which
        // made the ShadowDepth pass take down the engine on those devices.
        // We surface the failure as a clear log line so the caller can react.
        const u32 deviceMaxPushConstantSize = device.GetCapabilities().MaxPushConstantSize;
        if (desc.PushConstantSize > deviceMaxPushConstantSize)
        {
            XENGINE_LOG_ERROR(
                "Vulkan pipeline push constant size " +
                std::to_string(desc.PushConstantSize) +
                " exceeds device maxPushConstantsSize " +
                std::to_string(deviceMaxPushConstantSize));
            return;
        }

        XENGINE_ASSERT(desc.VertexShader != nullptr, "Vulkan graphics pipeline requires a vertex shader");
        XENGINE_ASSERT(&desc.VertexShader->GetOwnerDevice() == &device, "Vulkan graphics pipeline shaders must belong to the owning device");
        auto* vertexShader = CheckedVulkanCast<VulkanShader>(desc.VertexShader, device);
        VulkanShader* fragmentShader = desc.FragmentShader != nullptr
            ? CheckedVulkanCast<VulkanShader>(desc.FragmentShader, device)
            : nullptr;
        if (vertexShader == nullptr || !vertexShader->IsValid() ||
            (fragmentShader != nullptr && !fragmentShader->IsValid()))
        {
            XENGINE_LOG_ERROR("Vulkan graphics pipeline received an invalid shader");
            return;
        }

        XENGINE_LOG_INFO("Creating Vulkan pipeline layout");

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        descriptorSetLayouts.reserve(desc.BindGroupLayouts.size());
        for (RHIBindGroupLayout* layout : desc.BindGroupLayouts)
        {
            auto* vulkanLayout = CheckedVulkanCast<VulkanBindGroupLayout>(layout, device);
            if (vulkanLayout == nullptr || vulkanLayout->GetHandle() == VK_NULL_HANDLE)
            {
                XENGINE_LOG_ERROR("Vulkan pipeline received an invalid bind group layout");
                return;
            }

            descriptorSetLayouts.push_back(vulkanLayout->GetHandle());
        }

        VkPushConstantRange pushConstantRange {};
        pushConstantRange.stageFlags = m_PushConstantStages;
        pushConstantRange.offset = 0;
        pushConstantRange.size = desc.PushConstantSize;

        VkPipelineLayoutCreateInfo layoutCreateInfo {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCreateInfo.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size());
        layoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        if (desc.PushConstantSize > 0)
        {
            layoutCreateInfo.pushConstantRangeCount = 1;
            layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        }

        VkResult result = vkCreatePipelineLayout(m_Device, &layoutCreateInfo, nullptr, &m_PipelineLayout);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan pipeline layout: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return;
        }

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.push_back(VkPipelineShaderStageCreateInfo {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            vertexShader->GetVulkanStage(),
            vertexShader->GetHandle(),
            vertexShader->GetEntryPoint().c_str(),
            nullptr
        });
        if (fragmentShader != nullptr)
        {
            shaderStages.push_back(VkPipelineShaderStageCreateInfo {
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr,
                0,
                fragmentShader->GetVulkanStage(),
                fragmentShader->GetHandle(),
                fragmentShader->GetEntryPoint().c_str(),
                nullptr
            });
        }

        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        if (desc.VertexLayout.Stride > 0)
        {
            VkVertexInputBindingDescription binding {};
            binding.binding = 0;
            binding.stride = desc.VertexLayout.Stride;
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            bindings.push_back(binding);

            attributes.reserve(desc.VertexLayout.Attributes.size());
            for (const RHIVertexAttributeDesc& attributeDesc : desc.VertexLayout.Attributes)
            {
                VkVertexInputAttributeDescription attribute {};
                attribute.location = attributeDesc.Location;
                attribute.binding = 0;
                attribute.format = RHIFormatToVulkanFormat(attributeDesc.Format);
                attribute.offset = attributeDesc.Offset;
                attributes.push_back(attribute);
            }
        }

        VkPipelineVertexInputStateCreateInfo vertexInput {};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = static_cast<u32>(bindings.size());
        vertexInput.pVertexBindingDescriptions = bindings.data();
        vertexInput.vertexAttributeDescriptionCount = static_cast<u32>(attributes.size());
        vertexInput.pVertexAttributeDescriptions = attributes.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Vulkan requires that viewportCount and scissorCount be non-zero when
        // VK_DYNAMIC_STATE_VIEWPORT / VK_DYNAMIC_STATE_SCISSOR are set (the
        // _WITH_COUNT variants are what allow count=0). Both forward and
        // shadow-pipeline callers use non-zero counts here.
        VkPipelineViewportStateCreateInfo viewportState {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterization {};
        rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization.cullMode = VK_CULL_MODE_NONE;
        rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization.lineWidth = 1.0f;
        rasterization.depthBiasEnable = desc.EnableDepthBias ? VK_TRUE : VK_FALSE; 
        rasterization.depthBiasConstantFactor = desc.DepthBiasConstantFactor;
        rasterization.depthBiasClamp = desc.DepthBiasClamp;
        rasterization.depthBiasSlopeFactor = desc.DepthBiasSlopeFactor;

        VkPipelineMultisampleStateCreateInfo multisampling {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = desc.EnableDepthTest ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = desc.EnableDepthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState colorBlendAttachment {};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlend {};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = desc.HasColorAttachment ? 1u : 0u;
        colorBlend.pAttachments = desc.HasColorAttachment ? &colorBlendAttachment : nullptr;

        const std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        const VkFormat colorFormat = desc.HasColorAttachment
            ? RHIFormatToVulkanFormat(desc.ColorFormat)
            : VK_FORMAT_UNDEFINED;
        const VkFormat depthFormat = RHIFormatToVulkanFormat(desc.DepthFormat);
        if (desc.HasColorAttachment && colorFormat == VK_FORMAT_UNDEFINED)
        {
            XENGINE_LOG_ERROR("Vulkan graphics pipeline received an unsupported color format");
            return;
        }

        VkPipelineRenderingCreateInfo renderingCreateInfo {};
        renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingCreateInfo.colorAttachmentCount = desc.HasColorAttachment ? 1u : 0u;
        renderingCreateInfo.pColorAttachmentFormats = desc.HasColorAttachment
            ? &colorFormat
            : nullptr;
        renderingCreateInfo.depthAttachmentFormat = depthFormat;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext = &renderingCreateInfo;
        pipelineCreateInfo.stageCount = static_cast<u32>(shaderStages.size());
        pipelineCreateInfo.pStages = shaderStages.data();
        pipelineCreateInfo.pVertexInputState = &vertexInput;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterization;
        pipelineCreateInfo.pMultisampleState = &multisampling;
        pipelineCreateInfo.pDepthStencilState = &depthStencil;
        pipelineCreateInfo.pColorBlendState = &colorBlend;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.layout = m_PipelineLayout;
        pipelineCreateInfo.renderPass = VK_NULL_HANDLE;
        pipelineCreateInfo.subpass = 0;

        if (desc.DebugName != nullptr)
        {
            XENGINE_LOG_INFO(desc.DebugName);
        }

        result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_Pipeline);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan graphics pipeline: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
        }
    }

    VulkanPipeline::~VulkanPipeline()
    {
        if (m_Device != VK_NULL_HANDLE && m_Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_PipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
    }

    bool VulkanPipeline::IsValid() const
    {
        return m_Pipeline != VK_NULL_HANDLE && m_PipelineLayout != VK_NULL_HANDLE;
    }

    VkPipeline VulkanPipeline::GetHandle() const
    {
        return m_Pipeline;
    }

    VkPipelineLayout VulkanPipeline::GetLayout() const
    {
        return m_PipelineLayout;
    }

    VkShaderStageFlags VulkanPipeline::GetPushConstantStages() const
    {
        return m_PushConstantStages;
    }
}
