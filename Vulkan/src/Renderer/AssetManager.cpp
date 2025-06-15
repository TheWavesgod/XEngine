#include "AssetManager.h"

namespace LittleEngine
{
	Mesh* AssetManager::GetMeshByIndex(int index)
	{
		return AssetManager::Get().meshCache[index].get();
	}

	Material* AssetManager::GetMaterialByIndex(int index)
	{
		return AssetManager::Get().materialCache[index].get();
	}

	Material* AssetManager::LoadMaterialFromPath(const std::string& albedoPath, const std::string& normalPath,
		const std::string& metallicPath, const std::string& roughnessPath, const std::string& aoPath, const std::string& heightPath)
	{
		AssetManager::Get().materialCache.emplace_back(std::make_unique<Material>(albedoPath, normalPath, metallicPath, roughnessPath, aoPath, heightPath));
		return AssetManager::Get().materialCache.back().get();
	}

	void AssetManager::Initialize()
	{
		meshCache.push_back(std::unique_ptr<Mesh>(Mesh::GenerateTriangle()));
		meshCache.push_back(std::unique_ptr<Mesh>(Mesh::GenerateQuad()));
		meshCache.push_back(std::unique_ptr<Mesh>(Mesh::GenerateCube()));
		meshCache.push_back(std::unique_ptr<Mesh>(Mesh::GenerateFloor()));
		meshCache.push_back(std::unique_ptr<Mesh>(Mesh::GenerateSphere()));

		materialCache.emplace_back(std::make_unique<Material>(
			"StainlessSteel/used-stainless-steel2_albedo.png",
			"StainlessSteel/used-stainless-steel2_normal-dx.png",
			"StainlessSteel/used-stainless-steel2_metallic.png",
			"StainlessSteel/used-stainless-steel2_roughness.png",
			"StainlessSteel/used-stainless-steel2_ao.png",
			"StainlessSteel/used-stainless-steel2_height.png"
		)); // TODO: Something wrong with generating mipmap and check the desired format about material textures
	}
}