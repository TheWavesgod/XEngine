#pragma once

#include "VKEasyHeader.h"

namespace VK
{
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