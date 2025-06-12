#include "Level.h"

#include "vulkan/Shader.h"

namespace LittleEngine
{
	using namespace VK;

	void Level::Initialize()
	{

		VkDescriptorSetLayoutBinding globalCamUBDB = cam.GetCameraGlobalDescriptorSetLayoutBinding();

		VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &globalCamUBDB
		};
		globalUniformDescriptorSetLayout.Create(layoutInfo);

		descriptorPool.AllocateSets(globalUniformDescriptorSet, globalUniformDescriptorSetLayout);

		VkDescriptorBufferInfo bufferInfo = {
			.buffer = cam.GetGlobalCameraBuffer(),
			.offset = 0,
			.range = sizeof Camera::GlobalCameraData
		};

		globalUniformDescriptorSet.Write(bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		InitialScene();
	}


	void Level::UpdateObject(float dt)
	{
		cam.Update(dt);
	}

	void Level::InitialScene()
	{
		cam.GetCameraTransform().SetPosition({ 0.0f, 0.0f, 3.0f });
		cam.SetCameraYaw(0.0f);

		materials.emplace_back(std::make_shared<Material>(
			"StainlessSteel/used-stainless-steel2_albedo.png",
			"StainlessSteel/used-stainless-steel2_normal-dx.png",
			"StainlessSteel/used-stainless-steel2_metallic.png",
			"StainlessSteel/used-stainless-steel2_roughness.png",
			"StainlessSteel/used-stainless-steel2_ao.png",
			"StainlessSteel/used-stainless-steel2_height.png")
		);

		renderObjects.emplace_back(Mesh::GenerateCube(), materials[0]);
		renderObjects.emplace_back(Mesh::GenerateCube(), materials[0]);
		renderObjects.emplace_back(Mesh::GenerateCube(), materials[0]);

		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			renderObjects[i].SetPosition({ -3.0f + (float)(i * 2), 0.0f, -5.0f });
		}
	}
}
