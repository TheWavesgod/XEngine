#pragma once

#include "VkBase.h"

namespace VK
{
	class PipelineLayout
	{
		VkPipelineLayout handle = VK_NULL_HANDLE;

	public:
		PipelineLayout() = default;
		PipelineLayout(VkPipelineLayoutCreateInfo& createInfo) { Create(createInfo); }
		PipelineLayout(PipelineLayout&& other) noexcept { MoveHandle; }
		~PipelineLayout() { DestroyHandleBy(vkDestroyPipelineLayout); }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		// Non-const Function
		result_t Create(VkPipelineLayoutCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			VkResult result = vkCreatePipelineLayout(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ pipelineLayout ] ERROR\nFailed to create a pipeline layout!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
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
		~Pipeline() { DestroyHandleBy(vkDestroyPipeline); }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Non-const Function
		result_t Create(VkGraphicsPipelineCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			VkResult result = vkCreateGraphicsPipelines(VkBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ pipeline ] ERROR\nFailed to create a graphics pipeline!\nError code: {}\n", int32_t(result));
			}
			return result;
		}

		result_t Create(VkComputePipelineCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			VkResult result = vkCreateComputePipelines(VkBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ pipeline ] ERROR\nFailed to create a compute pipeline!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
	};
}