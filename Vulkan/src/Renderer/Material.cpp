#include "Material.h"

#include "Renderer.h"

namespace LittleEngine
{
	const std::string Material::defaultTexturePath = "../Resources/Materials/";

	Material::~Material()
	{
	}

	void Material::LoadTexture(const std::string& albedoPath,
		const std::string& normalPath,
		const std::string& metallicPath,
		const std::string& roughnessPath,
		const std::string& aoPath,
		const std::string& heightPath)
	{
		loadPaths = {
			albedoPath,
			normalPath,
			metallicPath,
			roughnessPath,
			aoPath,
			heightPath
		};

		VkSamplerCreateInfo samplerCreateInfo = Texture::MakeSamplerCreateInfo();
		for (size_t i = 0; i < static_cast<size_t>(TextureType::COUNT); ++i)
		{
			if (loadPaths[i].empty()) continue;

			std::string path = defaultTexturePath + loadPaths[i];
			textures[i].Create(path.c_str(), VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
			samplers[i].Create(samplerCreateInfo);

			materialFlagBits |= 1 << i;
		}

		materialFlagBitsBuffer.Create(sizeof(uint32_t));
		materialFlagBitsBuffer.TransferData(materialFlagBits);

		VkDescriptorSetLayoutBinding materialBindings[] = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // Albedo
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // Normal
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // AO
			{ 6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		};

		VkDescriptorSetLayoutCreateInfo materialLayoutCreateInfo = {
			.bindingCount = 7,
			.pBindings = materialBindings
		};
		pbrMaterialDefaultDescriptorSetLayout.Create(materialLayoutCreateInfo);

		Renderer::Get().GetGlobalDescriptorPool().AllocateSets(desciptorSet, pbrMaterialDefaultDescriptorSetLayout);

		// Update DescriptorSet
		VkDescriptorImageInfo imageInfo = {
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		
		for (size_t i = 0; i < static_cast<size_t>(TextureType::COUNT); ++i)
		{
			if (loadPaths[i].empty()) continue;

			imageInfo.sampler = samplers[i];
			imageInfo.imageView = textures[i].ImageViewRef();
			desciptorSet.Write(imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, i);
		}

		VkDescriptorBufferInfo bufferInfo = {
			.buffer = materialFlagBitsBuffer,
			.offset = 0,
			.range = sizeof(uint32_t)
		};
		desciptorSet.Write(bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6);
	}
}