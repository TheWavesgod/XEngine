#pragma once

#include <memory>
#include <vector>

#include "Mesh.h"
#include "Material.h"
#include "Vulkan/Shader.h"

namespace LittleEngine
{
	enum class DefaultMeshType
	{
		TRIANGLE = 0,
		QUAD,
		CUBE,
		FLOOR,
		SPHERE,

		TYPENUM
	};

	class AssetManager
	{
		friend class Mesh;
	public:
		static AssetManager& Get() { return am; }

		static Mesh* GetMeshByIndex(int index);
		static Material* GetMaterialByIndex(int index);

		static Material* LoadMaterialFromPath(
			const std::string& albedoPath,
			const std::string& normalPath = "",
			const std::string& metallicPath = "",
			const std::string& roughnessPath = "",
			const std::string& aoPath = "",
			const std::string& heightPath = "");

		void Initialize();

	private:
		static AssetManager am;
		
		std::vector<std::unique_ptr<ShaderModule>> shaderCache;
		std::vector<std::unique_ptr<Mesh>> meshCache;
		std::vector<std::unique_ptr<Material>> materialCache;
	};
	inline AssetManager AssetManager::am;
}