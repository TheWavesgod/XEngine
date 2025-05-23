#pragma once

#include "VKEasyHeader.h"

namespace VK
{
    struct GraphicsPipelineCreateInfoPack
    {
        VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        // Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

        std::vector<VkVertexInputBindingDescription> vertexInputBindings;
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;

        // Input Assembly 
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

        // Tessellation
        VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

        // Viewport
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

        std::vector<VkViewport> viewports;
        std::vector<VkRect2D> scissors;
        uint32_t dynamicViewportCount = 1; 
        uint32_t dynamicScissorCount = 1;

        // Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

        // Multisample
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

        // Depth & Stencil
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

        // Color Blend
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;

        // Dynamic
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };

        std::vector<VkDynamicState> dynamicStates;


        // -------------------------------------------------------------------------
        GraphicsPipelineCreateInfoPack()
        {
            SetCreateInfos();
            // if it's not a derived pipeline,createInfo.basePipelineIndex can't be 0, set -1
            createInfo.basePipelineIndex = -1;
        }

        GraphicsPipelineCreateInfoPack(const GraphicsPipelineCreateInfoPack& other) noexcept;

        //Getter
        operator VkGraphicsPipelineCreateInfo& () { return createInfo; }

        //Non-const Function
        void UpdateAllArrays()
        {
            createInfo.stageCount = shaderStages.size();
            vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputBindings.size();
            vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputAttributes.size();
            viewportStateCreateInfo.viewportCount = viewports.size() ? uint32_t(viewports.size()) : dynamicViewportCount;
            viewportStateCreateInfo.scissorCount = scissors.size() ? uint32_t(scissors.size()) : dynamicScissorCount;
            colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
            dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
            UpdateAllArrayAddresses();
        }

    private:
        void SetCreateInfos()
        {
            createInfo.pVertexInputState = &vertexInputStateCreateInfo;
            createInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
            createInfo.pTessellationState = &tessellationStateCreateInfo;
            createInfo.pViewportState = &viewportStateCreateInfo;
            createInfo.pRasterizationState = &rasterizationStateCreateInfo;
            createInfo.pMultisampleState = &multisampleStateCreateInfo;
            createInfo.pDepthStencilState = &depthStencilStateCreateInfo;
            createInfo.pColorBlendState = &colorBlendStateCreateInfo;
            createInfo.pDynamicState = &dynamicStateCreateInfo;
        }

        void UpdateAllArrayAddresses()
        {
            createInfo.pStages = shaderStages.data();
            vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindings.data();
            vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();
            viewportStateCreateInfo.pViewports = viewports.data();
            viewportStateCreateInfo.pScissors = scissors.data();
            colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();
            dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
        }
    };

	/*
	 * Layout contains the way of how pipeline uses descriptors and push constant
	 */
	class PipelineLayout
	{
		VkPipelineLayout handle = VK_NULL_HANDLE;

	public:
		PipelineLayout() = default;
		PipelineLayout(VkPipelineLayoutCreateInfo& createInfo) { Create(createInfo); }

		PipelineLayout(PipelineLayout&& other) noexcept { MoveHandle; }
		~PipelineLayout();

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		// Non-const Function
		result_t Create(VkPipelineLayoutCreateInfo& createInfo);
	};


	/**
	 * Abstraction of the process of processing data
	 */
	class Pipeline
	{
		VkPipeline handle = VK_NULL_HANDLE;

	public:
		Pipeline() = default;
		Pipeline(VkGraphicsPipelineCreateInfo& createInfo) { Create(createInfo); }
		Pipeline(VkComputePipelineCreateInfo& createInfo) { Create(createInfo); }

		Pipeline(Pipeline&& other) noexcept { MoveHandle; }
		~Pipeline();

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Non-const Function
		result_t Create(VkGraphicsPipelineCreateInfo& createInfo);
		result_t Create(VkComputePipelineCreateInfo& createInfo);
	};
}