#include "Pipeline.h"
#include "VkBase.h"

namespace VK
{
	PipelineLayout::~PipelineLayout()
	{
		DestroyHandleBy(vkDestroyPipelineLayout);
	}

	result_t PipelineLayout::Create(VkPipelineLayoutCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkResult result = vkCreatePipelineLayout(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ pipelineLayout ] ERROR\nFailed to create a pipeline layout!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	Pipeline::~Pipeline()
	{
		DestroyHandleBy(vkDestroyPipeline);
	}

	result_t Pipeline::Create(VkGraphicsPipelineCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		VkResult result = vkCreateGraphicsPipelines(VkBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ pipeline ] ERROR\nFailed to create a graphics pipeline!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Pipeline::Create(VkComputePipelineCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		VkResult result = vkCreateComputePipelines(VkBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ pipeline ] ERROR\nFailed to create a compute pipeline!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	GraphicsPipelineCreateInfoPack::GraphicsPipelineCreateInfoPack(const GraphicsPipelineCreateInfoPack& other) noexcept
	{
		createInfo = other.createInfo;
		SetCreateInfos();

		vertexInputStateCreateInfo = other.vertexInputStateCreateInfo;
		inputAssemblyStateCreateInfo = other.inputAssemblyStateCreateInfo;
		tessellationStateCreateInfo = other.tessellationStateCreateInfo;
		viewportStateCreateInfo = other.viewportStateCreateInfo;
		rasterizationStateCreateInfo = other.rasterizationStateCreateInfo;
		multisampleStateCreateInfo = other.multisampleStateCreateInfo;
		depthStencilStateCreateInfo = other.depthStencilStateCreateInfo;
		colorBlendStateCreateInfo = other.colorBlendStateCreateInfo;
		dynamicStateCreateInfo = other.dynamicStateCreateInfo;

		shaderStages = other.shaderStages;
		vertexInputBindings = other.vertexInputBindings;
		vertexInputAttributes = other.vertexInputAttributes;
		viewports = other.viewports;
		scissors = other.scissors;
		colorBlendAttachmentStates = other.colorBlendAttachmentStates;
		dynamicStates = other.dynamicStates;
		UpdateAllArrayAddresses();
	}
}