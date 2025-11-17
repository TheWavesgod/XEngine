#include "Pipeline.h"
#include "VkBase.h"

namespace VK
{
	PipelineLayout::~PipelineLayout()
	{
		DestroyHandleBy(vkDestroyPipelineLayout);
	}

	PipelineLayout::Builder& PipelineLayout::Builder::SetFlags(VkPipelineLayoutCreateFlags flags)
	{
		flags_ = flags;
		return *this;
	}

	PipelineLayout::Builder& PipelineLayout::Builder::AddDescriptorSetLayout(VkDescriptorSetLayout layout)
	{
		setLayouts_.push_back(layout);
		return *this;
	}

	PipelineLayout::Builder& PipelineLayout::Builder::AddPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size)
	{
		VkPushConstantRange range{};
		range.stageFlags = stageFlags;
		range.offset = offset;
		range.size = size;
		pushConstants_.push_back(range);
		return *this;
	}

	PipelineLayout PipelineLayout::Builder::Build()
	{
		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.flags = flags_;
		info.setLayoutCount = static_cast<uint32_t>(setLayouts_.size());
		info.pSetLayouts = setLayouts_.empty() ? nullptr : setLayouts_.data();
		info.pushConstantRangeCount = static_cast<uint32_t>(pushConstants_.size());
		info.pPushConstantRanges = pushConstants_.empty() ? nullptr : pushConstants_.data();

		VkPipelineLayout layout = VK_NULL_HANDLE;
		if (vkCreatePipelineLayout(device_, &info, nullptr, &layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout!");

		return PipelineLayout();
	}

	Pipeline::~Pipeline()
	{
		DestroyHandleBy(vkDestroyPipeline);
	}

	GraphicsPipeline::Builder::Builder(VkDevice device) : device_(device) 
	{
		// Default settings
		inputAssembly_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly_.primitiveRestartEnable = VK_FALSE;

		rasterization_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization_.lineWidth = 1.0f;

		multisample_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_.minSampleShading = 1.0f;

		depthStencil_.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		colorBlend_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::AddShaderStage(VkShaderStageFlagBits stage, VkShaderModule module, const char* entryName)
	{
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = stage;
		stageInfo.module = module;
		stageInfo.pName = entryName;
		shaderStages_.push_back(stageInfo);
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::AddVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
	{
		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = binding;
		vertexBinding.stride = stride;
		vertexBinding.inputRate = inputRate;
		vertexBindings_.push_back(vertexBinding);
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::AddVertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription vertexAttribute{};
		vertexAttribute.location = location;
		vertexAttribute.binding = binding;
		vertexAttribute.format = format;
		vertexAttribute.offset = offset;
		vertexAttributes_.push_back(vertexAttribute);
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetVertexInputState(const VkPipelineVertexInputStateCreateInfo& info)
	{
		customVertexInput_ = info;
		useCustomVertexInput_ = true;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetInputAssembly(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
	{
		inputAssembly_.topology = topology;
		inputAssembly_.primitiveRestartEnable = primitiveRestartEnable;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetViewport(const VkViewport& viewport)
	{
		viewport_ = viewport;
		hasViewport_ = true;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetScissor(const VkRect2D& scissor) 
	{
		scissor_ = scissor;
		hasScissor_ = true;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetRasterizationState(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace)
	{
		rasterization_.polygonMode = polygonMode;
		rasterization_.cullMode = cullMode;
		rasterization_.frontFace = frontFace;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetDepthClamp(VkBool32 enable)
	{
		rasterization_.depthClampEnable = enable;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetSampleCount(VkSampleCountFlagBits samples)
	{
		multisample_.rasterizationSamples = samples;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::EnableDepthTest(
		VkBool32 enableTest, VkBool32 enableWrite, VkCompareOp compareOp)
	{
		depthStencil_.depthTestEnable = enableTest;
		depthStencil_.depthWriteEnable = enableWrite;
		depthStencil_.depthCompareOp = compareOp;
		depthStencil_.depthBoundsTestEnable = VK_FALSE;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::AddColorBlendAttachment(
		VkBool32 blendEnable, VkColorComponentFlags colorWriteMask)
	{
		VkPipelineColorBlendAttachmentState a{};
		a.colorWriteMask = colorWriteMask;
		a.blendEnable = blendEnable;

		if (blendEnable) {
			a.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			a.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			a.colorBlendOp = VK_BLEND_OP_ADD;
			a.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			a.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			a.alphaBlendOp = VK_BLEND_OP_ADD;
		}

		colorAttachments_.push_back(a);
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::AddDynamicState(VkDynamicState state)
	{
		dynamicStates_.push_back(state);
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetPipelineLayout(VkPipelineLayout layout)
	{
		layout_ = layout;
		return *this;
	}

	GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetRenderPass(VkRenderPass renderPass, uint32_t subpass)
	{
		renderPass_ = renderPass;
		subpass_ = subpass;
		return *this;
	}

	GraphicsPipeline GraphicsPipeline::Builder::Build()
	{
		if (layout_ == VK_NULL_HANDLE) 
			throw std::runtime_error("GraphicsPipeline::Builder: pipeline layout is not set.");
		
		if (renderPass_ == VK_NULL_HANDLE) 
			throw std::runtime_error("GraphicsPipeline::Builder: render pass is not set.");
		
		if (shaderStages_.empty()) 
			throw std::runtime_error("GraphicsPipeline::Builder: no shader stages.");

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		if (useCustomVertexInput_)
		{
			vertexInput = customVertexInput_;
		}
		else
		{
			vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings_.size());
			vertexInput.pVertexBindingDescriptions = vertexBindings_.empty() ? nullptr : vertexBindings_.data();
			vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes_.size());
			vertexInput.pVertexAttributeDescriptions = vertexAttributes_.empty() ? nullptr : vertexAttributes_.data();
		}
		
		// viewport state
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = hasViewport_ ? 1u : 0u;
		viewportState.pViewports = hasViewport_ ? &viewport_ : nullptr;
		viewportState.scissorCount = hasScissor_ ? 1u : 0u;
		viewportState.pScissors = hasScissor_ ? &scissor_ : nullptr;

		// color blend
		colorBlend_.attachmentCount = static_cast<uint32_t>(colorAttachments_.size());
		colorBlend_.pAttachments = colorAttachments_.empty() ? nullptr : colorAttachments_.data();

		// dynamic state
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates_.size());
		dynamicState.pDynamicStates = dynamicStates_.empty() ? nullptr : dynamicStates_.data();

		VkGraphicsPipelineCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.stageCount = static_cast<uint32_t>(shaderStages_.size());
		info.pStages = shaderStages_.data();
		info.pVertexInputState = &vertexInput;
		info.pInputAssemblyState = &inputAssembly_;
		info.pViewportState = &viewportState;
		info.pRasterizationState = &rasterization_;
		info.pMultisampleState = &multisample_;
		info.pDepthStencilState = &depthStencil_;
		info.pColorBlendState = &colorBlend_;
		info.pDynamicState = dynamicStates_.empty() ? nullptr : &dynamicState;
		info.layout = layout_;
		info.renderPass = renderPass_;
		info.subpass = subpass_;

		VkPipeline pipeline = VK_NULL_HANDLE;
		if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE,1, &info,nullptr, &pipeline) != VK_SUCCESS)
			throw std::runtime_error("GraphicsPipeline::Builder: failed to create graphics pipeline.");
	
		return GraphicsPipeline(pipeline);
	}

	ComputePipeline::Builder& ComputePipeline::Builder::SetShader(
		VkShaderModule module, const char* entryName)
	{
		stage_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stage_.module = module;
		stage_.pName = entryName;
		return *this;
	}

	ComputePipeline::Builder& ComputePipeline::Builder::SetFlags(VkPipelineCreateFlags flags)
	{
		flags_ = flags;
		return *this;
	}

	ComputePipeline ComputePipeline::Builder::build()
	{
		if (layout_ == VK_NULL_HANDLE) 
			throw std::runtime_error("ComputePipeline::Builder: pipeline layout is not set.");
		
		if (stage_.module == VK_NULL_HANDLE)
			throw std::runtime_error("ComputePipeline::Builder: shader module is not set.");

		VkComputePipelineCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.flags = flags_;
		info.stage = stage_;
		info.layout = layout_;

		VkPipeline pipeline = VK_NULL_HANDLE;
		if(vkCreateComputePipelines(device_, VK_NULL_HANDLE,1, &info,nullptr, &pipeline) != VK_SUCCESS)
			throw std::runtime_error("ComputePipeline::Builder: failed to create compute pipeline.");

		return ComputePipeline(pipeline);
	}
}