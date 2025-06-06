#include "RenderObject.h"

#include "Vulkan/VkBase.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Descriptor.h"
#include "Vulkan/RenderPass.h"

#include "gtc/type_ptr.hpp"

namespace LittleEngine
{
	void RenderObject::Draw(VkCommandBuffer commandBuffer, const DescriptorSet& globalSet) const  
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, Mesh::VERTEX, 1, mesh->GetVertexBuffers()[Mesh::VERTEX].Address(), &offset);
		vkCmdBindVertexBuffers(commandBuffer, Mesh::TEXCOORD, 1, mesh->GetVertexBuffers()[Mesh::TEXCOORD].Address(), &offset);
		vkCmdBindVertexBuffers(commandBuffer, Mesh::NORMAL, 1, mesh->GetVertexBuffers()[Mesh::NORMAL].Address(), &offset);
		vkCmdBindVertexBuffers(commandBuffer, Mesh::TANGENT, 1, mesh->GetVertexBuffers()[Mesh::TANGENT].Address(), &offset);
		vkCmdBindVertexBuffers(commandBuffer, Mesh::BiTANGENT, 1, mesh->GetVertexBuffers()[Mesh::BiTANGENT].Address(), &offset);
		vkCmdBindVertexBuffers(commandBuffer, Mesh::COLOUR, 1, mesh->GetVertexBuffers()[Mesh::COLOUR].Address(), &offset);

		if (mesh->UseIndices())
		{
			vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}

		vkCmdPushConstants(commandBuffer, pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(glm::mat4), glm::value_ptr(transform.GetTransMatrix()));

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout, 0, 1, globalSet.Address(), 0, nullptr);

		if (mesh->UseIndices())
		{
			vkCmdDrawIndexed(commandBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(commandBuffer, mesh->GetVertexCount(), 1, 0, 0);
		}
	}

	void RenderObject::Create(const RenderPass& rp, const DescriptorSetLayout& gdsl)
	{
		const VkExtent2D& windowSize = VkBase::Base().SwapchainCreateInfo().imageExtent;

		ShaderModule vert("GeneralPBR.vert");
		ShaderModule frag("GeneralPBR.frag");

		VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2] = {
			vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
			frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// Push constant
		VkPushConstantRange pushRange = {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(glm::mat4)
		};

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			.setLayoutCount = 1,
			.pSetLayouts = gdsl.Address(),
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushRange  
		};

		pipelineLayout.Create(pipelineLayoutCreateInfo);

		GraphicsPipelineCreateInfoPack pipelineCiPack = {};
		
		pipelineCiPack.createInfo.layout = pipelineLayout;
		pipelineCiPack.createInfo.renderPass = rp;
		pipelineCiPack.vertexInputBindings = mesh->GetVertexInputBindings();
		pipelineCiPack.vertexInputAttributes = mesh->GetVertexInputAttributes();
		pipelineCiPack.inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);

		pipelineCiPack.rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCiPack.rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		pipelineCiPack.multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		pipelineCiPack.depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

		pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
		pipelineCiPack.UpdateAllArrays();

		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos;

		pipeline.Create(pipelineCiPack);
	}
}


