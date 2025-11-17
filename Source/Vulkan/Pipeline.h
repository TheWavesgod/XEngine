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
        PipelineLayout(VkPipelineLayout layout) : handle(layout) {}

		PipelineLayout(PipelineLayout&& other) noexcept { MoveHandle; }
		~PipelineLayout();

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

        PipelineLayout(const PipelineLayout&) = delete;
        PipelineLayout& operator=(const PipelineLayout&) = delete;

    public:
        class Builder
        {
        public:
            explicit Builder(VkDevice device) : device_(device) {}

            Builder& SetFlags(VkPipelineLayoutCreateFlags flags);
            Builder& AddDescriptorSetLayout(VkDescriptorSetLayout layout);
            Builder& AddPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
            
            template <typename T>
            Builder& AddPushConstantRangeTyped(VkShaderStageFlags stageFlags, uint32_t offset = 0)
            {
                return addPushConstantRange(stageFlags, offset, sizeof(T));
            }

            PipelineLayout Build();

        private:
            VkDevice device_;
            VkPipelineLayoutCreateFlags flags_ = 0;
            std::vector<VkDescriptorSetLayout> setLayouts_;
            std::vector<VkPushConstantRange> pushConstants_;
        };
	};


	/**
	 * Abstraction of the process of processing data
	 */
	class Pipeline
	{
    protected:
		VkPipeline handle = VK_NULL_HANDLE;

	public:
		Pipeline() = default;
        Pipeline(VkPipeline pipeline) : handle(pipeline) {}

		Pipeline(Pipeline&& other) noexcept { MoveHandle; }
		~Pipeline();

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
	};

    class GraphicsPipeline : public Pipeline
    {
    public:
        using Pipeline::Pipeline;

        class Builder
        {
        public:
            explicit Builder(VkDevice device);

            Builder& AddShaderStage(
                VkShaderStageFlagBits stage,
                VkShaderModule module,
                const char* entryName = "main");

            Builder& AddVertexBinding(
                uint32_t binding,
                uint32_t stride,
                VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);

            Builder& AddVertexAttribute(
                uint32_t location,
                uint32_t binding,
                VkFormat format,
                uint32_t offset);

            Builder& SetVertexInputState(const VkPipelineVertexInputStateCreateInfo& info); 

            Builder& SetInputAssembly(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable = VK_FALSE);

            Builder& SetViewport(const VkViewport& viewport);

            Builder& SetScissor(const VkRect2D& scissor);

            Builder& SetRasterizationState(
                VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
                VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
                VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE);

            Builder& SetDepthClamp(VkBool32 enable);

            Builder& SetSampleCount(VkSampleCountFlagBits samples);

            Builder& EnableDepthTest(
                VkBool32 enableTest = true, 
                VkBool32 enableWrite = true, 
                VkCompareOp compareOp = VK_COMPARE_OP_LESS);

            Builder& AddColorBlendAttachment(
                VkBool32 blendEnable,
                VkColorComponentFlags colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT);

            Builder& AddDynamicState(VkDynamicState state);

            Builder& SetPipelineLayout(VkPipelineLayout layout);
            Builder& SetRenderPass(VkRenderPass renderPass, uint32_t subpass = 0);

            GraphicsPipeline Build();

        private:
            VkDevice device_ = VK_NULL_HANDLE;

            // Object Ref
            VkPipelineLayout layout_ = VK_NULL_HANDLE;
            VkRenderPass     renderPass_ = VK_NULL_HANDLE;
            uint32_t         subpass_ = 0;

            // Shader 
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages_;

            // Vertex input
            std::vector<VkVertexInputBindingDescription> vertexBindings_;
            std::vector<VkVertexInputAttributeDescription> vertexAttributes_;
            VkPipelineVertexInputStateCreateInfo customVertexInput_{};
            bool useCustomVertexInput_ = false;

            // Fixed function status
            VkPipelineInputAssemblyStateCreateInfo  inputAssembly_{};
            VkPipelineRasterizationStateCreateInfo  rasterization_{};
            VkPipelineMultisampleStateCreateInfo    multisample_{};
            VkPipelineDepthStencilStateCreateInfo   depthStencil_{};
            VkPipelineColorBlendStateCreateInfo     colorBlend_{};

            std::vector<VkPipelineColorBlendAttachmentState> colorAttachments_;
            std::vector<VkDynamicState> dynamicStates_;

            // viewport / scissor
            VkViewport viewport_{};
            VkRect2D   scissor_{};
            bool hasViewport_ = false;
            bool hasScissor_ = false ;
        };
    };

    class ComputePipeline : public Pipeline
    {
    public:
        using Pipeline::Pipeline;

        class Builder 
        {
        public:
            explicit Builder(VkDevice device) : device_(device) {}

            Builder& SetPipelineLayout(VkPipelineLayout layout);
            Builder& SetShader( VkShaderModule module, const char* entryName = "main");
            Builder& SetFlags(VkPipelineCreateFlags flags);

            ComputePipeline build();

        private:
            VkDevice device_ = VK_NULL_HANDLE;

            VkPipelineLayout layout_ = VK_NULL_HANDLE;
            VkPipelineShaderStageCreateInfo stage_;
            VkPipelineCreateFlags flags_ = 0;
        };
    };

}