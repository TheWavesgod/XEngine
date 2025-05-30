#include "RenderObject.h"

#include "Vulkan/Shader.h"

#include "gtc/type_ptr.hpp"

namespace LittleEngine
{
	void RenderObject::Draw(VkCommandBuffer commandBuffer) const
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		
		std::array<VkDeviceSize, 6> offsets = { 0, 0, 0, 0, 0, 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 
			static_cast<uint32_t>(mesh->GetVertexBuffers().size()), 
			mesh->GetVertexBuffers().data(),
			offsets.data());
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdPushConstants(commandBuffer, pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof glm::mat4, glm::value_ptr(transform.GetTransMatrix()));

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout, 0, 1, );

		vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
	}

	void RenderObject::Create()
	{
		ShaderModule vert("GeneralPBR.vert");
		ShaderModule frag("GeneralPBR.frag");

		VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2] = {
			vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
			frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// Push constant
		VkPushConstantRange pushRange = {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof glm::mat4
		};

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			.setLayoutCount = 1,
			//.pSetLayouts
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushRange
		};

		pipelineLayout.Create(pipelineLayoutCreateInfo);

		GraphicsPipelineCreateInfoPack pipelineCreateInfoPack = {};
		
		pipelineCreateInfoPack.createInfo.layout = pipelineLayout;
		pipelineCreateInfoPack.inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		//pipelineCreateInfoPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		//pipelineCreateInfoPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCreateInfoPack.multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCreateInfoPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });

		pipeline.Create(pipelineCreateInfoPack);
	}
}


