#pragma once

#include <memory>
#include <vector>

#include "Mesh.h"
#include "Material.h"

namespace LittleEngine
{
	class AssetManager
	{
		class MaterialManager 
		{
			std::vector<std::shared_ptr<Material>> materials;
		};

		class MeshManager
		{
			std::vector<std::shared_ptr<Mesh>> meshes;
		};
	};
}