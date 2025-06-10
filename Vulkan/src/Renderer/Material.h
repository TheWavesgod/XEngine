#pragma once

#include "Vulkan/Texture.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Descriptor.h"

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

		const DescriptorSetLayout& GetDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		std::array<std::string, static_cast<size_t>(TextureType::COUNT)> loadPaths;

		std::array<Texture2d, static_cast<size_t>(TextureType::COUNT)> textures;
		std::array<Sampler, static_cast<size_t>(TextureType::COUNT)> samplers;

		DescriptorSet desciptorSet;
		DescriptorSetLayout descriptorSetLayout;
	};
}