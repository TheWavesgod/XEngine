#include "Scene.h"

#include "AssetManager.h"
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

		Mesh* cube = AssetManager::GetMeshByIndex(static_cast<int>(DefaultMeshType::CUBE));
		Material* stainlessSteel = AssetManager::GetMaterialByIndex(0);
		renderObjects.emplace_back(cube, stainlessSteel);
		renderObjects.emplace_back(cube, stainlessSteel);
		renderObjects.emplace_back(cube, nullptr);

		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			renderObjects[i].Create();
			renderObjects[i].SetPosition({ -3.0f + (float)(i * 2), 0.0f, -5.0f });
		}
	}
}
