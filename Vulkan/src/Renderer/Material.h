#pragma once

#include "Vulkan/Texture.h"
#include "Vulkan/Sampler.h"

namespace LittleEngine
{
	using namespace VK;
	

	class Material
	{
	public:
		enum class TextureType
		{
			Albedo = 0,
			Normal,
			Metallic,
			Roughness,
			AO,
			Height,

			COUNT
		};

		Material();
		~Material();

		void LoadTexture(const std::string& path);

	private:
		std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureType::COUNT)> textures;
		std::array<Sampler, static_cast<size_t>(TextureType::COUNT)> samplers;
	};
}