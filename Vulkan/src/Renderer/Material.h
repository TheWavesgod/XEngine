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

		Material() = default;
		Material(const std::string& albedoPath,
			const std::string& normalPath = "",
			const std::string& metallicPath = "",
			const std::string& roughnessPath = "",
			const std::string& aoPath = "",
			const std::string& heightPath = "") 
		{
			LoadTexture(albedoPath, normalPath, metallicPath, roughnessPath, aoPath, heightPath);
		}

		~Material();

		void LoadTexture(const std::string& albedoPath,
			const std::string& normalPath = "",
			const std::string& metallicPath = "",
			const std::string& roughnessPath = "",
			const std::string& aoPath = "",
			const std::string& heightPath = "");

		static const DescriptorSetLayout& GetPbrMaterialDefaultDescriptorSetLayout() { return pbrMaterialDefaultDescriptorSetLayout; }

	private:
		std::array<std::string, static_cast<size_t>(TextureType::COUNT)> loadPaths;

		std::array<Texture2d, static_cast<size_t>(TextureType::COUNT)> textures;
		std::array<Sampler, static_cast<size_t>(TextureType::COUNT)> samplers;

		static const std::string defaultTexturePath;
		
		static inline DescriptorSetLayout pbrMaterialDefaultDescriptorSetLayout;

		/*enum MaterialFlagBits {
			USE_ALBEDO		= 1 << 0,
			USE_NORMAL		= 1 << 1,
			USE_METALLIC	= 1 << 2,
			USE_ROUGHNESS	= 1 << 3,
			USE_AO			= 1 << 4,
			USE_HEIGHT		= 1 << 5
		};*/
		UniformBuffer materialFlagBitsBuffer;
		uint32_t materialFlagBits = 0;

		DescriptorSet desciptorSet;
	};
}