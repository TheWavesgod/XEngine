#pragma once

#include "Vulkan/GlfwGeneral.hpp"
#include "Vulkan/Synchronization.h"
#include "Vulkan/Command.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Descriptor.h"
#include "Vulkan/Shader.h"

using namespace VK;

PipelineLayout pipelineLayout_triangle;
Pipeline pipeline_triangle;

DescriptorSetLayout descriptorSetLayout_texture;
PipelineLayout pipelineLayout_texture;

struct vertex
{
    glm::vec2 position;
    glm::vec4 color;
};

vertex vertices[] = {
		{ { -.5f, -.5f }, { 1, 1, 0, 1 } },
		{ {  .5f, -.5f }, { 1, 0, 0, 1 } },
		{ { -.5f,  .5f }, { 0, 1, 0, 1 } },
		{ {  .5f,  .5f }, { 0, 0, 1, 1 } }
};
VertexBuffer vertexBuffer;


uint16_t indices[] = {
	0, 1, 2,
	1, 2, 3 };
IndexBuffer indexBuffer;

void CreateLayout()
{
    VkPushConstantRange pushConstantRange = {
        VK_SHADER_STAGE_VERTEX_BIT,
        0,  // offset
        24
    };

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_texture = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_texture = {
        .bindingCount = 1,
        .pBindings = &descriptorSetLayoutBinding_texture
    };
    descriptorSetLayout_texture.Create(descriptorSetLayoutCreateInfo_texture);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .setLayoutCount = 1,
        .pSetLayouts = descriptorSetLayout_texture.Address()
    };
    pipelineLayout_texture.Create(pipelineLayoutCreateInfo);

    /*VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);*/
}

void CreatePipeline(const RenderPass& rp, const VkExtent2D& windowSize)
{
	static ShaderModule vert("FirstTriangle.vert");
	static ShaderModule frag("FirstTriangle.frag");

    static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
        vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
        frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };

	GraphicsPipelineCreateInfoPack pipelineCreateInfoPack = {};

	// Data comes from layout 0 vertex buffer, input per vertex
	pipelineCreateInfoPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
	// location is 0, data comes from layout 0£¬vec2 referes to VK_FORMAT_R32G32_SFLOAT, use offsetof to calculate the start position in struct vertex
	pipelineCreateInfoPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, position));
	// location is 2, vec4 refers to VK_FORMAT_R32G32B32A32_SFLOAT
	pipelineCreateInfoPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex, color));

	pipelineCreateInfoPack.createInfo.layout = pipelineLayout_texture;
	pipelineCreateInfoPack.createInfo.renderPass = rp;
	pipelineCreateInfoPack.inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfoPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
	pipelineCreateInfoPack.scissors.emplace_back(VkOffset2D{}, windowSize);
	pipelineCreateInfoPack.multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineCreateInfoPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
	pipelineCreateInfoPack.UpdateAllArrays();
	pipelineCreateInfoPack.createInfo.stageCount = 2;
	pipelineCreateInfoPack.createInfo.pStages = shaderStageCreateInfos_triangle;

	pipeline_triangle.Create(pipelineCreateInfoPack);
}

void CreateRenderObject()
{
    vertexBuffer.Create(sizeof vertices);
    vertexBuffer.TransferData(vertices);

    indexBuffer.Create(sizeof indices);
    indexBuffer.TransferData(indices);
}