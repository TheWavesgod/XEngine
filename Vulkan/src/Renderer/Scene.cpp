#include "Scene.h"

#include "vulkan/Shader.h"

namespace LittleEngine
{
	using namespace VK;

	void Scene::Initialize()
	{
		InitialScene();
	}


	void Scene::UpdateObject(float dt)
	{
		cam.Update(dt);
	}

	void Scene::InitialScene()
	{
		cam.GetCameraTransform().SetPosition({ 0.0f, 0.0f, 3.0f });
		cam.SetCameraYaw(0.0f);

		/*materials.emplace_back(std::make_shared<Material>(
			"StainlessSteel/used-stainless-steel2_albedo.png",
			"StainlessSteel/used-stainless-steel2_normal-dx.png",
			"StainlessSteel/used-stainless-steel2_metallic.png",
			"StainlessSteel/used-stainless-steel2_roughness.png",
			"StainlessSteel/used-stainless-steel2_ao.png",
			"StainlessSteel/used-stainless-steel2_height.png")
		);*/

		renderObjects.emplace_back(Mesh::GenerateCube(), nullptr);
		renderObjects.emplace_back(Mesh::GenerateCube(), nullptr);
		renderObjects.emplace_back(Mesh::GenerateCube(), nullptr);

		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			renderObjects[i].Create();
			renderObjects[i].SetPosition({ -3.0f + (float)(i * 2), 0.0f, -5.0f });
		}
	}
}
